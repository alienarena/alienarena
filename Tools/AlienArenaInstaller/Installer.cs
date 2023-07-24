using AlienArenaInstaller.Properties;
using System;
using System.Windows.Forms;
using SharpSvn;
using System.Diagnostics;
using System.Net;
using System.IO;

namespace AlienArenaInstaller
{
	public partial class Installer : Form
	{
		// Approximately 1 GB
		const double _totalDownloadSize = 1 * 1024 * 1024 * 1024;
		string _latestRelease = Settings.Default.Release;

		public Installer()
		{
			InitializeComponent();

			// Get latest release
			using (var versionClient = new WebClient())
			{
				var versionUrl = "http://alienarena.org/version/crx_version";
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
					txtRelease.Text = Settings.Default.Release;
				}
			}

			txtInstallationFolder.Text = Settings.Default.InstallationFolder;
			folderBrowser.SelectedPath = Settings.Default.InstallationFolder;
			chkLatestUnstable.Checked = Settings.Default.LatestUnstable;
			txtWidth.Text = Settings.Default.Width;
			txtHeight.Text = Settings.Default.Height;
			chkFullscreen.Checked = Settings.Default.Fullscreen;
		}

		private void btnInstall_Click(object sender, EventArgs e)
		{
			if (MessageBox.Show("The installer will now download all files from svn, this may take five to fifteen minutes. After that it will run the OpenAL installer, if needed. Do you want to continue?", "Alien Arena Installer", MessageBoxButtons.YesNoCancel) == DialogResult.Yes) 
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

						SetStatusText($"Finished downloading.");
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

		private void InstallOpenAL()
		{
			if (!File.Exists(@"C:\Windows\System32\OpenAL32.dll"))
			{
				var p = new Process();
				p.StartInfo.WorkingDirectory = txtInstallationFolder.Text;
				p.StartInfo.FileName = "oalinst.exe";
				p.Start();				
			}
		}
		private void StartAlienArena(bool confirm = false)
		{
			SaveSettings();

			if (!File.Exists(Path.Combine(txtInstallationFolder.Text, "alienarena.exe")))
			{
				MessageBox.Show("First press Install to install Alien Arena!", "Alien Arena Installer", MessageBoxButtons.OK);
				return;
			}

			if (!confirm || MessageBox.Show("Alien Arena is up-to-date. Do you want to run Alien Arena now?", "Alien Arena Installer", MessageBoxButtons.YesNoCancel) == DialogResult.Yes)
			{
				var scr = Screen.PrimaryScreen;
				var fullscreen = chkFullscreen.Checked ? "1" : "0";
				if (txtWidth.Text != Convert.ToString(scr.Bounds.Width) || txtHeight.Text != Convert.ToString(scr.Bounds.Height))
				{
					fullscreen = "0";
				}
				var parameters = $"+set vid_width {txtWidth.Text} +set vid_height {txtHeight.Text} +set vid_fullscreen {fullscreen};";

				// TODO: find out how to adjust scaling to 100% before starting

				var p = new Process();
				p.StartInfo.WorkingDirectory = txtInstallationFolder.Text;
				p.StartInfo.FileName = "alienarena.exe";
				p.StartInfo.Arguments = parameters;
				p.Start();
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
			if (chkFullscreen.Checked)
			{
				var scr = Screen.PrimaryScreen;
				txtWidth.Text = Convert.ToString(scr.Bounds.Width);
				txtHeight.Text = Convert.ToString(scr.Bounds.Height);
			}
		}
	}
}
