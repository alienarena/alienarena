namespace AlienArenaInstaller
{
	partial class Installer
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Installer));
			this.lblExplanation = new System.Windows.Forms.Label();
			this.folderBrowser = new System.Windows.Forms.FolderBrowserDialog();
			this.statusStrip = new System.Windows.Forms.StatusStrip();
			this.toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.toolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
			this.statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.panelResolution = new System.Windows.Forms.Panel();
			this.lblDisplay = new System.Windows.Forms.Label();
			this.cmbScreen = new System.Windows.Forms.ComboBox();
			this.btnPlay = new System.Windows.Forms.Button();
			this.lblHeight = new System.Windows.Forms.Label();
			this.lblWidth = new System.Windows.Forms.Label();
			this.lblResolutionOptions = new System.Windows.Forms.Label();
			this.txtWidth = new System.Windows.Forms.TextBox();
			this.chkFullscreen = new System.Windows.Forms.CheckBox();
			this.txtHeight = new System.Windows.Forms.TextBox();
			this.panelInstallationOptions = new System.Windows.Forms.Panel();
			this.btnInstall = new System.Windows.Forms.Button();
			this.lblInstallationOptions = new System.Windows.Forms.Label();
			this.chkLatestUnstable = new System.Windows.Forms.CheckBox();
			this.lblRelease = new System.Windows.Forms.Label();
			this.txtRelease = new System.Windows.Forms.TextBox();
			this.btnSelectFolder = new System.Windows.Forms.Button();
			this.lblInstallationFolder = new System.Windows.Forms.Label();
			this.txtInstallationFolder = new System.Windows.Forms.TextBox();
			this.linkAlienArena = new System.Windows.Forms.LinkLabel();
			this.lblInstallerVersion = new System.Windows.Forms.Label();
			this.logo = new System.Windows.Forms.Panel();
			this.statusStrip.SuspendLayout();
			this.panelResolution.SuspendLayout();
			this.panelInstallationOptions.SuspendLayout();
			this.SuspendLayout();
			// 
			// lblExplanation
			// 
			this.lblExplanation.AutoSize = true;
			this.lblExplanation.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(100)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))));
			this.lblExplanation.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblExplanation.ForeColor = System.Drawing.SystemColors.InactiveCaption;
			this.lblExplanation.Location = new System.Drawing.Point(12, 51);
			this.lblExplanation.Name = "lblExplanation";
			this.lblExplanation.Size = new System.Drawing.Size(312, 24);
			this.lblExplanation.TabIndex = 2;
			this.lblExplanation.Text = "Install, update or launch Alien Arena";
			// 
			// statusStrip
			// 
			this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel,
            this.toolStripProgressBar});
			this.statusStrip.Location = new System.Drawing.Point(0, 556);
			this.statusStrip.Name = "statusStrip";
			this.statusStrip.Size = new System.Drawing.Size(914, 22);
			this.statusStrip.TabIndex = 8;
			// 
			// toolStripStatusLabel
			// 
			this.toolStripStatusLabel.Name = "toolStripStatusLabel";
			this.toolStripStatusLabel.Size = new System.Drawing.Size(0, 17);
			// 
			// toolStripProgressBar
			// 
			this.toolStripProgressBar.Name = "toolStripProgressBar";
			this.toolStripProgressBar.RightToLeft = System.Windows.Forms.RightToLeft.No;
			this.toolStripProgressBar.Size = new System.Drawing.Size(200, 16);
			// 
			// statusLabel
			// 
			this.statusLabel.Name = "statusLabel";
			this.statusLabel.Size = new System.Drawing.Size(0, 17);
			// 
			// panelResolution
			// 
			this.panelResolution.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(200)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))));
			this.panelResolution.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.panelResolution.Controls.Add(this.lblDisplay);
			this.panelResolution.Controls.Add(this.cmbScreen);
			this.panelResolution.Controls.Add(this.btnPlay);
			this.panelResolution.Controls.Add(this.lblHeight);
			this.panelResolution.Controls.Add(this.lblWidth);
			this.panelResolution.Controls.Add(this.lblResolutionOptions);
			this.panelResolution.Controls.Add(this.txtWidth);
			this.panelResolution.Controls.Add(this.chkFullscreen);
			this.panelResolution.Controls.Add(this.txtHeight);
			this.panelResolution.Location = new System.Drawing.Point(12, 370);
			this.panelResolution.Name = "panelResolution";
			this.panelResolution.Size = new System.Drawing.Size(890, 131);
			this.panelResolution.TabIndex = 13;
			// 
			// lblDisplay
			// 
			this.lblDisplay.AutoSize = true;
			this.lblDisplay.BackColor = System.Drawing.Color.Transparent;
			this.lblDisplay.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblDisplay.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.lblDisplay.Location = new System.Drawing.Point(301, 43);
			this.lblDisplay.Name = "lblDisplay";
			this.lblDisplay.RightToLeft = System.Windows.Forms.RightToLeft.No;
			this.lblDisplay.Size = new System.Drawing.Size(60, 20);
			this.lblDisplay.TabIndex = 20;
			this.lblDisplay.Text = "Display";
			// 
			// cmbScreen
			// 
			this.cmbScreen.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.cmbScreen.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.cmbScreen.FormattingEnabled = true;
			this.cmbScreen.Location = new System.Drawing.Point(381, 44);
			this.cmbScreen.MinimumSize = new System.Drawing.Size(120, 0);
			this.cmbScreen.Name = "cmbScreen";
			this.cmbScreen.Size = new System.Drawing.Size(249, 23);
			this.cmbScreen.TabIndex = 7;
			this.cmbScreen.SelectedIndexChanged += new System.EventHandler(this.cmbScreen_SelectedIndexChanged);
			// 
			// btnPlay
			// 
			this.btnPlay.BackColor = System.Drawing.SystemColors.GradientInactiveCaption;
			this.btnPlay.Font = new System.Drawing.Font("Microsoft Sans Serif", 20.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnPlay.ForeColor = System.Drawing.Color.SteelBlue;
			this.btnPlay.Location = new System.Drawing.Point(668, 58);
			this.btnPlay.Name = "btnPlay";
			this.btnPlay.Size = new System.Drawing.Size(205, 45);
			this.btnPlay.TabIndex = 2;
			this.btnPlay.Text = "Play";
			this.btnPlay.UseVisualStyleBackColor = false;
			this.btnPlay.Click += new System.EventHandler(this.btnPlay_Click);
			// 
			// lblHeight
			// 
			this.lblHeight.AutoSize = true;
			this.lblHeight.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblHeight.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.lblHeight.Location = new System.Drawing.Point(3, 83);
			this.lblHeight.Name = "lblHeight";
			this.lblHeight.Size = new System.Drawing.Size(56, 20);
			this.lblHeight.TabIndex = 18;
			this.lblHeight.Text = "Height";
			// 
			// lblWidth
			// 
			this.lblWidth.AutoSize = true;
			this.lblWidth.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblWidth.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.lblWidth.Location = new System.Drawing.Point(3, 43);
			this.lblWidth.Name = "lblWidth";
			this.lblWidth.Size = new System.Drawing.Size(50, 20);
			this.lblWidth.TabIndex = 17;
			this.lblWidth.Text = "Width";
			// 
			// lblResolutionOptions
			// 
			this.lblResolutionOptions.AutoSize = true;
			this.lblResolutionOptions.BackColor = System.Drawing.Color.DimGray;
			this.lblResolutionOptions.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblResolutionOptions.ForeColor = System.Drawing.SystemColors.ControlLight;
			this.lblResolutionOptions.Location = new System.Drawing.Point(-1, 0);
			this.lblResolutionOptions.Name = "lblResolutionOptions";
			this.lblResolutionOptions.Size = new System.Drawing.Size(109, 18);
			this.lblResolutionOptions.TabIndex = 16;
			this.lblResolutionOptions.Text = "Launch options";
			// 
			// txtWidth
			// 
			this.txtWidth.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.txtWidth.Location = new System.Drawing.Point(160, 44);
			this.txtWidth.MaxLength = 5;
			this.txtWidth.Name = "txtWidth";
			this.txtWidth.Size = new System.Drawing.Size(59, 21);
			this.txtWidth.TabIndex = 5;
			this.txtWidth.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
			this.txtWidth.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.txtWidth_KeyPress);
			// 
			// chkFullscreen
			// 
			this.chkFullscreen.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.chkFullscreen.AutoSize = true;
			this.chkFullscreen.BackColor = System.Drawing.Color.Transparent;
			this.chkFullscreen.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.chkFullscreen.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.chkFullscreen.Location = new System.Drawing.Point(525, 81);
			this.chkFullscreen.Name = "chkFullscreen";
			this.chkFullscreen.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.chkFullscreen.Size = new System.Drawing.Size(105, 24);
			this.chkFullscreen.TabIndex = 8;
			this.chkFullscreen.Text = "Full screen";
			this.chkFullscreen.UseVisualStyleBackColor = false;
			this.chkFullscreen.CheckedChanged += new System.EventHandler(this.chkFullscreen_CheckedChanged);
			// 
			// txtHeight
			// 
			this.txtHeight.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.txtHeight.Location = new System.Drawing.Point(160, 81);
			this.txtHeight.MaxLength = 5;
			this.txtHeight.Name = "txtHeight";
			this.txtHeight.Size = new System.Drawing.Size(59, 21);
			this.txtHeight.TabIndex = 6;
			this.txtHeight.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
			this.txtHeight.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.txtHeight_KeyPress);
			// 
			// panelInstallationOptions
			// 
			this.panelInstallationOptions.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(200)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))));
			this.panelInstallationOptions.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.panelInstallationOptions.Controls.Add(this.btnInstall);
			this.panelInstallationOptions.Controls.Add(this.lblInstallationOptions);
			this.panelInstallationOptions.Controls.Add(this.chkLatestUnstable);
			this.panelInstallationOptions.Controls.Add(this.lblRelease);
			this.panelInstallationOptions.Controls.Add(this.txtRelease);
			this.panelInstallationOptions.Controls.Add(this.btnSelectFolder);
			this.panelInstallationOptions.Controls.Add(this.lblInstallationFolder);
			this.panelInstallationOptions.Controls.Add(this.txtInstallationFolder);
			this.panelInstallationOptions.Location = new System.Drawing.Point(12, 221);
			this.panelInstallationOptions.Name = "panelInstallationOptions";
			this.panelInstallationOptions.Size = new System.Drawing.Size(890, 131);
			this.panelInstallationOptions.TabIndex = 14;
			// 
			// btnInstall
			// 
			this.btnInstall.BackColor = System.Drawing.SystemColors.GradientInactiveCaption;
			this.btnInstall.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
			this.btnInstall.Font = new System.Drawing.Font("Microsoft Sans Serif", 20.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnInstall.ForeColor = System.Drawing.Color.SteelBlue;
			this.btnInstall.Location = new System.Drawing.Point(668, 58);
			this.btnInstall.Name = "btnInstall";
			this.btnInstall.Size = new System.Drawing.Size(205, 45);
			this.btnInstall.TabIndex = 1;
			this.btnInstall.Text = "Install";
			this.btnInstall.UseVisualStyleBackColor = false;
			this.btnInstall.Click += new System.EventHandler(this.btnInstall_Click);
			// 
			// lblInstallationOptions
			// 
			this.lblInstallationOptions.AutoSize = true;
			this.lblInstallationOptions.BackColor = System.Drawing.Color.DimGray;
			this.lblInstallationOptions.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblInstallationOptions.ForeColor = System.Drawing.SystemColors.ControlLight;
			this.lblInstallationOptions.Location = new System.Drawing.Point(-1, 0);
			this.lblInstallationOptions.Name = "lblInstallationOptions";
			this.lblInstallationOptions.Size = new System.Drawing.Size(130, 18);
			this.lblInstallationOptions.TabIndex = 14;
			this.lblInstallationOptions.Text = "Installation options";
			// 
			// chkLatestUnstable
			// 
			this.chkLatestUnstable.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.chkLatestUnstable.AutoSize = true;
			this.chkLatestUnstable.BackColor = System.Drawing.Color.Transparent;
			this.chkLatestUnstable.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.chkLatestUnstable.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.chkLatestUnstable.Location = new System.Drawing.Point(482, 44);
			this.chkLatestUnstable.Name = "chkLatestUnstable";
			this.chkLatestUnstable.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.chkLatestUnstable.Size = new System.Drawing.Size(148, 24);
			this.chkLatestUnstable.TabIndex = 3;
			this.chkLatestUnstable.Text = "Latest (unstable)";
			this.chkLatestUnstable.UseVisualStyleBackColor = false;
			this.chkLatestUnstable.CheckedChanged += new System.EventHandler(this.chkLatestUnstable_CheckedChanged);
			// 
			// lblRelease
			// 
			this.lblRelease.AutoSize = true;
			this.lblRelease.BackColor = System.Drawing.Color.Transparent;
			this.lblRelease.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblRelease.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.lblRelease.Location = new System.Drawing.Point(3, 51);
			this.lblRelease.Name = "lblRelease";
			this.lblRelease.Size = new System.Drawing.Size(68, 20);
			this.lblRelease.TabIndex = 13;
			this.lblRelease.Text = "Release";
			// 
			// txtRelease
			// 
			this.txtRelease.Enabled = false;
			this.txtRelease.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.txtRelease.Location = new System.Drawing.Point(160, 44);
			this.txtRelease.Name = "txtRelease";
			this.txtRelease.Size = new System.Drawing.Size(59, 21);
			this.txtRelease.TabIndex = 8;
			this.txtRelease.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
			// 
			// btnSelectFolder
			// 
			this.btnSelectFolder.BackColor = System.Drawing.SystemColors.ButtonFace;
			this.btnSelectFolder.BackgroundImage = global::AlienArenaInstaller.Properties.Resources.browsefolder;
			this.btnSelectFolder.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
			this.btnSelectFolder.FlatAppearance.BorderSize = 0;
			this.btnSelectFolder.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnSelectFolder.ForeColor = System.Drawing.SystemColors.ActiveCaption;
			this.btnSelectFolder.Location = new System.Drawing.Point(598, 81);
			this.btnSelectFolder.Name = "btnSelectFolder";
			this.btnSelectFolder.Size = new System.Drawing.Size(32, 22);
			this.btnSelectFolder.TabIndex = 4;
			this.btnSelectFolder.UseVisualStyleBackColor = false;
			this.btnSelectFolder.Click += new System.EventHandler(this.btnSelectFolder_Click);
			// 
			// lblInstallationFolder
			// 
			this.lblInstallationFolder.AutoSize = true;
			this.lblInstallationFolder.BackColor = System.Drawing.Color.Transparent;
			this.lblInstallationFolder.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblInstallationFolder.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.lblInstallationFolder.Location = new System.Drawing.Point(3, 86);
			this.lblInstallationFolder.Name = "lblInstallationFolder";
			this.lblInstallationFolder.Size = new System.Drawing.Size(130, 20);
			this.lblInstallationFolder.TabIndex = 12;
			this.lblInstallationFolder.Text = "Installation folder";
			// 
			// txtInstallationFolder
			// 
			this.txtInstallationFolder.Enabled = false;
			this.txtInstallationFolder.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.txtInstallationFolder.Location = new System.Drawing.Point(160, 81);
			this.txtInstallationFolder.Name = "txtInstallationFolder";
			this.txtInstallationFolder.Size = new System.Drawing.Size(432, 21);
			this.txtInstallationFolder.TabIndex = 10;
			// 
			// linkAlienArena
			// 
			this.linkAlienArena.ActiveLinkColor = System.Drawing.SystemColors.ControlLightLight;
			this.linkAlienArena.AutoSize = true;
			this.linkAlienArena.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(100)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))));
			this.linkAlienArena.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.linkAlienArena.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.linkAlienArena.LinkBehavior = System.Windows.Forms.LinkBehavior.HoverUnderline;
			this.linkAlienArena.LinkColor = System.Drawing.SystemColors.ControlLightLight;
			this.linkAlienArena.Location = new System.Drawing.Point(154, 522);
			this.linkAlienArena.Name = "linkAlienArena";
			this.linkAlienArena.Size = new System.Drawing.Size(164, 16);
			this.linkAlienArena.TabIndex = 8;
			this.linkAlienArena.TabStop = true;
			this.linkAlienArena.Text = "https://www.alienarena.org";
			this.linkAlienArena.VisitedLinkColor = System.Drawing.SystemColors.ControlLightLight;
			this.linkAlienArena.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkAlienArena_LinkClicked);
			// 
			// lblInstallerVersion
			// 
			this.lblInstallerVersion.AutoSize = true;
			this.lblInstallerVersion.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(100)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))));
			this.lblInstallerVersion.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblInstallerVersion.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.lblInstallerVersion.Location = new System.Drawing.Point(9, 522);
			this.lblInstallerVersion.Name = "lblInstallerVersion";
			this.lblInstallerVersion.Size = new System.Drawing.Size(139, 16);
			this.lblInstallerVersion.TabIndex = 16;
			this.lblInstallerVersion.Text = "Version 1.0.0.7  © 2023";
			// 
			// logo
			// 
			this.logo.BackgroundImage = global::AlienArenaInstaller.Properties.Resources.logo;
			this.logo.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
			this.logo.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.logo.Location = new System.Drawing.Point(12, 9);
			this.logo.Name = "logo";
			this.logo.Size = new System.Drawing.Size(312, 42);
			this.logo.TabIndex = 1;
			// 
			// Installer
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.BackgroundImage = global::AlienArenaInstaller.Properties.Resources.alienarena9;
			this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
			this.ClientSize = new System.Drawing.Size(914, 578);
			this.Controls.Add(this.lblExplanation);
			this.Controls.Add(this.logo);
			this.Controls.Add(this.lblInstallerVersion);
			this.Controls.Add(this.linkAlienArena);
			this.Controls.Add(this.panelInstallationOptions);
			this.Controls.Add(this.panelResolution);
			this.Controls.Add(this.statusStrip);
			this.ForeColor = System.Drawing.SystemColors.ActiveCaptionText;
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.Name = "Installer";
			this.Text = "Alien Arena Installer";
			this.Closed += new System.EventHandler(this.Installer_Closed);
			this.statusStrip.ResumeLayout(false);
			this.statusStrip.PerformLayout();
			this.panelResolution.ResumeLayout(false);
			this.panelResolution.PerformLayout();
			this.panelInstallationOptions.ResumeLayout(false);
			this.panelInstallationOptions.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion
		private System.Windows.Forms.Label lblExplanation;
		private System.Windows.Forms.FolderBrowserDialog folderBrowser;
		private System.Windows.Forms.StatusStrip statusStrip;
		private System.Windows.Forms.ToolStripStatusLabel statusLabel;
		private System.Windows.Forms.Panel panelResolution;
		private System.Windows.Forms.Label lblWidth;
		private System.Windows.Forms.Label lblResolutionOptions;
		private System.Windows.Forms.TextBox txtWidth;
		private System.Windows.Forms.CheckBox chkFullscreen;
		private System.Windows.Forms.TextBox txtHeight;
		private System.Windows.Forms.Label lblHeight;
		private System.Windows.Forms.Panel panelInstallationOptions;
		private System.Windows.Forms.Label lblInstallationOptions;
		private System.Windows.Forms.CheckBox chkLatestUnstable;
		private System.Windows.Forms.Label lblRelease;
		private System.Windows.Forms.TextBox txtRelease;
		private System.Windows.Forms.Button btnSelectFolder;
		private System.Windows.Forms.Label lblInstallationFolder;
		private System.Windows.Forms.TextBox txtInstallationFolder;
		private System.Windows.Forms.Button btnPlay;
		private System.Windows.Forms.Button btnInstall;
		private System.Windows.Forms.ToolStripProgressBar toolStripProgressBar;
		private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel;
		private System.Windows.Forms.LinkLabel linkAlienArena;
		private System.Windows.Forms.Label lblInstallerVersion;
		private System.Windows.Forms.Panel logo;
		private System.Windows.Forms.Label lblDisplay;
		private System.Windows.Forms.ComboBox cmbScreen;
	}
}

