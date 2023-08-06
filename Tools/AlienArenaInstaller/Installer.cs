using AlienArenaInstaller.Properties;
using System;
using System.Windows.Forms;
using SharpSvn;
using System.Diagnostics;
using System.Net;
using System.IO;
using System.Threading;

namespace AlienArenaInstaller
{
	public partial class Installer : Form
	{
		// Approximately 1 GB
		const double _totalDownloadSize = 1 * 1024 * 1024 * 1024;
		string _latestRelease = Settings.Default.Release;
		float _screenScalingFactor;

		public Installer()
		{
			InitializeComponent();

			// Get latest version number
			using (var versionClient = new WebClient())
			{
				var versionUrl = "https://alienarena.org/version/crx_version";
				try
				{
					var stream = versionClient.OpenRead(versionUrl);
					var newRelease = new StreamReader(stream)?.ReadToEnd();
					if (!string.IsNullOrWhiteSpace(newRelease))
					{
						_latestRelease = newRelease.Trim();
						txtRelease.Text = _latestRelease;
					}
				}
				catch
				{
					SetStatusText("An error occurred fetching the latest version number.");
					txtRelease.Text = Settings.Default.Release;
				}
			}

			txtInstallationFolder.Text = Settings.Default.InstallationFolder;
			folderBrowser.SelectedPath = Settings.Default.InstallationFolder;
			chkLatestUnstable.Checked = Settings.Default.LatestUnstable;
			txtWidth.Text = Settings.Default.Width;
			txtHeight.Text = Settings.Default.Height;
			chkFullscreen.Checked = Settings.Default.Fullscreen;

			if (IsInstalled()) {
				btnInstall.Text = "Update";
			}

			System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
			FileVersionInfo fvi = FileVersionInfo.GetVersionInfo(assembly.Location);
			lblInstallerVersion.Text = $"Version {fvi.FileVersion}   © 2023 ";
			linkAlienArena.Left = lblInstallerVersion.Left + lblInstallerVersion.Width;

			_screenScalingFactor = GetScreenScalingFactor();

			foreach (var screen in Screen.AllScreens)
			{
				cmbScreen.Items.Add($"{screen.FriendlyDeviceName()} - {screen.Bounds.Width} x {screen.Bounds.Height}");
			}
			if (Settings.Default.Screen < cmbScreen.Items.Count)
			{
				cmbScreen.SelectedIndex = Settings.Default.Screen;
			}
			else
			{
				cmbScreen.SelectedIndex = 0;
			}
		}

		private void Installer_Closed(object sender, EventArgs e)
		{
			SaveSettings();
		}

		private float GetScreenScalingFactor()
		{
			float dx = 0.0f, dy = 0.0f;
			using (var g = this.CreateGraphics())
			{
				try
				{
					dx = g.DpiX;
					dy = g.DpiY;
				}
				catch
				{
				}
			}
			return dx > 0.0f ? dx / 96.0f : 1.0f;
		}

		private void btnInstall_Click(object sender, EventArgs e)
		{
			var continueMessage = "";
			if (!IsInstalled()) {
				continueMessage = "The installer will now download all files from svn, this may take five to fifteen minutes.";
			} else {
				continueMessage = "The installer will now download any new and modified files from svn, this may take a couple of minutes.";
			}
			if (!IsOpenALInstalled()) {
				continueMessage += " After that it will run the OpenAL audio library installer.";
			}
			continueMessage += " Do you want to continue?";
			
			if (MessageBox.Show(continueMessage, "Alien Arena Installer", MessageBoxButtons.YesNoCancel) == DialogResult.Yes)
			{
				string url;
				if (chkLatestUnstable.Checked)
				{
					url = "svn://svn.icculus.org/alienarena/trunk";
					SetStatusText("Downloading latest Alien Arena (unstable) ...");
				}
				else
				{
					url = $"svn://svn.icculus.org/alienarena/tags/{txtRelease.Text}";
					SetStatusText($"Downloading Alien Arena release {txtRelease.Text} ...");
				}

				toolStripProgressBar.Value = 0;
				Cursor.Current = Cursors.WaitCursor;

				var downloadSucceeded = false;
				// Uses SharpSvn, see also https://github.com/AmpScm/SharpSvn
				using (var client = new SvnClient())
				{
					client.Progress += new EventHandler<SvnProgressEventArgs>(OnProgress);

					Action doCheckOut = delegate ()
					{
						client.CheckOut(new SvnUriTarget(url), txtInstallationFolder.Text);

						SetStatusText("Finished downloading.");
						toolStripProgressBar.Value = 100;
						downloadSucceeded = true;
					};

					try
					{
						doCheckOut();
					}
					catch (SvnObstructedUpdateException)
					{
						// Probably the url changed, so switch url and retry
						client.Switch(txtInstallationFolder.Text, new SvnUriTarget(url));

						try
						{
							toolStripProgressBar.Value = 0;
							doCheckOut();
						}
						catch (SvnException ex)
						{
							MessageBox.Show(ex.Message, "Alien Arena Installer", MessageBoxButtons.OK);
							SetStatusText("An error occurred.");
						}
					}
					catch (SvnException ex)
					{
						MessageBox.Show(ex.Message, "Alien Arena Installer", MessageBoxButtons.OK);
						SetStatusText("An error occurred.");
					}
					finally
					{
						Cursor.Current = Cursors.Default;
					}

					if (downloadSucceeded)
					{
						InstallOpenAL();
						StartAlienArena(confirm: true);
					}
				}
			}

			SaveSettings();
		}

		private void btnPlay_Click(object sender, EventArgs e)
		{
			StartAlienArena();
		}

		private void OnProgress(object sender, SvnProgressEventArgs e)
    	{
			var progress = (e.Progress / _totalDownloadSize) * 100.0;

			if (progress <= toolStripProgressBar.Maximum)
			{
				toolStripProgressBar.Value = (int)progress;
			}
			
			Application.DoEvents();
		}

		private void SetStatusText(string message)
		{
			toolStripStatusLabel.Text = message;
			statusStrip.Update();
		}

		private bool IsInstalled() {
			return File.Exists(Path.Combine(txtInstallationFolder.Text, "alienarena.exe"));
		}

		private bool IsOpenALInstalled() {
			return File.Exists(@"C:\Windows\System32\OpenAL32.dll");
		}

		private void InstallOpenAL()
		{
			if (!IsOpenALInstalled())
			{
				var p = new Process();
				p.StartInfo.WorkingDirectory = txtInstallationFolder.Text;
				p.StartInfo.FileName = "oalinst.exe";
				p.Start();
				while (!p.HasExited)
				{
					// Wait
				}

				SetStatusText("Installed OpenAL audio library.");
			}
		}

		private void StartAlienArena(bool confirm = false)
		{
			SaveSettings();

			if (!IsInstalled())
			{
				MessageBox.Show("First press Install to install Alien Arena!", "Alien Arena Installer", MessageBoxButtons.OK);
				return;
			}

			if (!confirm || MessageBox.Show("Alien Arena is up-to-date. Do you want to run Alien Arena now?", "Alien Arena Installer", MessageBoxButtons.YesNoCancel) == DialogResult.Yes)
			{
				SetStatusText("");
				toolStripProgressBar.Value = 0;

				if (IsValid())
				{
					var scr = Screen.AllScreens[cmbScreen.SelectedIndex];
					var fullscreen = chkFullscreen.Checked ? "1" : "0";
					if (txtWidth.Text != Convert.ToString(scr.Bounds.Width) || txtHeight.Text != Convert.ToString(scr.Bounds.Height))
					{
						fullscreen = "0";
					}
					var parameters = $"+set vid_width {txtWidth.Text} +set vid_height {txtHeight.Text} +set vid_fullscreen {fullscreen};";

					var p = new Process();
					p.StartInfo.WorkingDirectory = txtInstallationFolder.Text;
					p.StartInfo.FileName = "alienarena.exe";
					p.StartInfo.Arguments = parameters;
					p.Start();

					// A sleep is needed to make it move the window
					Thread.Sleep(2000);
					Screen.AllScreens[cmbScreen.SelectedIndex].Show(p.MainWindowHandle);
				}
			}
		}

		private void SaveSettings()
		{
			Settings.Default.Release = txtRelease.Text;
			Settings.Default.InstallationFolder = txtInstallationFolder.Text;
			Settings.Default.LatestUnstable = chkLatestUnstable.Checked;
			Settings.Default.Width = txtWidth.Text;
			Settings.Default.Height = txtHeight.Text;
			Settings.Default.Fullscreen = chkFullscreen.Checked;
			Settings.Default.Screen = cmbScreen.SelectedIndex;
			Settings.Default.Save();
		}

		private void btnSelectFolder_Click(object sender, EventArgs e)
		{
			DialogResult result = folderBrowser.ShowDialog();
			if (result == DialogResult.OK)
			{
				txtInstallationFolder.Text = folderBrowser.SelectedPath;
			}
		}

		private void chkLatestUnstable_CheckedChanged(object sender, EventArgs e)
		{
			if (chkLatestUnstable.Checked)
			{
				txtRelease.Text = "";
			}
			else if (txtRelease.Text.Length == 0)
			{
				txtRelease.Text = _latestRelease;
			}
		}

		private void chkFullscreen_CheckedChanged(object sender, EventArgs e)
		{
			AdjustForFullScreen();
		}

		private void cmbScreen_SelectedIndexChanged(object sender, EventArgs e)
		{
			AdjustForFullScreen();
		}

		private void AdjustForFullScreen()
		{
			if (chkFullscreen.Checked && cmbScreen.SelectedIndex >= 0)
			{
				var scr = Screen.AllScreens[cmbScreen.SelectedIndex];
				txtWidth.Text = Convert.ToString(scr.Bounds.Width);
				txtHeight.Text = Convert.ToString(scr.Bounds.Height);
			}
		}

		private void linkAlienArena_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
		{
			Process.Start("https://www.alienarena.org");
		}

		private void txtWidth_KeyPress(object sender, KeyPressEventArgs e)
		{
			AllowOnlyDigits(e);
		}

		private void txtHeight_KeyPress(object sender, KeyPressEventArgs e)
		{
			AllowOnlyDigits(e);
		}

		private void AllowOnlyDigits(KeyPressEventArgs e)
		{
			if (!char.IsControl(e.KeyChar) && !char.IsDigit(e.KeyChar))
			{
				e.Handled = true;
			}
		}

		private bool IsValid()
		{
			var errorMessage = "";

			if (txtWidth.Text.ToInteger() < 640)
			{
				errorMessage += "Width must be at least 640.";
			}

			if (txtHeight.Text.ToInteger() < 480)
			{
				if (errorMessage.Length > 0)
				{
					errorMessage += "\n";
				}
				errorMessage += "Height must be at least 480.";
			}

			if (errorMessage.Length > 0)
			{
				MessageBox.Show(errorMessage, "Alien Arena Installer", MessageBoxButtons.OK);
				return false;
			}

			return true;
		}
	}
}
