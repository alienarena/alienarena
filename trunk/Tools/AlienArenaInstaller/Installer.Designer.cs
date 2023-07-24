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
			this.logo = new System.Windows.Forms.Panel();
			this.lblExplanation = new System.Windows.Forms.Label();
			this.folderBrowser = new System.Windows.Forms.FolderBrowserDialog();
			this.statusStrip = new System.Windows.Forms.StatusStrip();
			this.statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.panelResolution = new System.Windows.Forms.Panel();
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
			this.toolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
			this.toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.statusStrip.SuspendLayout();
			this.panelResolution.SuspendLayout();
			this.panelInstallationOptions.SuspendLayout();
			this.SuspendLayout();
			// 
			// logo
			// 
			this.logo.BackgroundImage = global::AlienArenaInstaller.Properties.Resources.logo;
			this.logo.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
			this.logo.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.logo.Location = new System.Drawing.Point(12, 12);
			this.logo.Name = "logo";
			this.logo.Size = new System.Drawing.Size(294, 42);
			this.logo.TabIndex = 1;
			// 
			// lblExplanation
			// 
			this.lblExplanation.AutoSize = true;
			this.lblExplanation.BackColor = System.Drawing.Color.Transparent;
			this.lblExplanation.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblExplanation.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.lblExplanation.Location = new System.Drawing.Point(8, 57);
			this.lblExplanation.Name = "lblExplanation";
			this.lblExplanation.Size = new System.Drawing.Size(603, 24);
			this.lblExplanation.TabIndex = 2;
			this.lblExplanation.Text = "This program will download and install the latest release of Alien Arena.";
			// 
			// statusStrip
			// 
			this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel,
            this.toolStripProgressBar});
			this.statusStrip.Location = new System.Drawing.Point(0, 528);
			this.statusStrip.Name = "statusStrip";
			this.statusStrip.Size = new System.Drawing.Size(914, 22);
			this.statusStrip.TabIndex = 8;
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
			this.panelResolution.Controls.Add(this.btnPlay);
			this.panelResolution.Controls.Add(this.lblHeight);
			this.panelResolution.Controls.Add(this.lblWidth);
			this.panelResolution.Controls.Add(this.lblResolutionOptions);
			this.panelResolution.Controls.Add(this.txtWidth);
			this.panelResolution.Controls.Add(this.chkFullscreen);
			this.panelResolution.Controls.Add(this.txtHeight);
			this.panelResolution.Location = new System.Drawing.Point(12, 400);
			this.panelResolution.Name = "panelResolution";
			this.panelResolution.Size = new System.Drawing.Size(890, 112);
			this.panelResolution.TabIndex = 13;
			// 
			// btnPlay
			// 
			this.btnPlay.BackColor = System.Drawing.SystemColors.GradientInactiveCaption;
			this.btnPlay.Font = new System.Drawing.Font("Microsoft Sans Serif", 20.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnPlay.ForeColor = System.Drawing.Color.SteelBlue;
			this.btnPlay.Location = new System.Drawing.Point(668, 48);
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
			this.lblHeight.Location = new System.Drawing.Point(3, 71);
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
			this.lblResolutionOptions.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblResolutionOptions.ForeColor = System.Drawing.SystemColors.ControlLight;
			this.lblResolutionOptions.Location = new System.Drawing.Point(-1, 0);
			this.lblResolutionOptions.Name = "lblResolutionOptions";
			this.lblResolutionOptions.Size = new System.Drawing.Size(72, 16);
			this.lblResolutionOptions.TabIndex = 16;
			this.lblResolutionOptions.Text = "Resolution";
			// 
			// txtWidth
			// 
			this.txtWidth.Location = new System.Drawing.Point(148, 45);
			this.txtWidth.Name = "txtWidth";
			this.txtWidth.Size = new System.Drawing.Size(59, 20);
			this.txtWidth.TabIndex = 5;
			// 
			// chkFullscreen
			// 
			this.chkFullscreen.AutoSize = true;
			this.chkFullscreen.BackColor = System.Drawing.Color.Transparent;
			this.chkFullscreen.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.chkFullscreen.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.chkFullscreen.Location = new System.Drawing.Point(446, 43);
			this.chkFullscreen.Name = "chkFullscreen";
			this.chkFullscreen.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.chkFullscreen.Size = new System.Drawing.Size(105, 24);
			this.chkFullscreen.TabIndex = 7;
			this.chkFullscreen.Text = "Full screen";
			this.chkFullscreen.UseVisualStyleBackColor = false;
			this.chkFullscreen.CheckedChanged += new System.EventHandler(this.chkFullscreen_CheckedChanged);
			// 
			// txtHeight
			// 
			this.txtHeight.Location = new System.Drawing.Point(148, 73);
			this.txtHeight.Name = "txtHeight";
			this.txtHeight.Size = new System.Drawing.Size(59, 20);
			this.txtHeight.TabIndex = 6;
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
			this.panelInstallationOptions.Location = new System.Drawing.Point(12, 251);
			this.panelInstallationOptions.Name = "panelInstallationOptions";
			this.panelInstallationOptions.Size = new System.Drawing.Size(890, 133);
			this.panelInstallationOptions.TabIndex = 14;
			// 
			// btnInstall
			// 
			this.btnInstall.BackColor = System.Drawing.SystemColors.GradientInactiveCaption;
			this.btnInstall.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
			this.btnInstall.Font = new System.Drawing.Font("Microsoft Sans Serif", 20.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnInstall.ForeColor = System.Drawing.Color.SteelBlue;
			this.btnInstall.Location = new System.Drawing.Point(668, 63);
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
			this.lblInstallationOptions.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblInstallationOptions.ForeColor = System.Drawing.SystemColors.ControlLight;
			this.lblInstallationOptions.Location = new System.Drawing.Point(-1, 0);
			this.lblInstallationOptions.Name = "lblInstallationOptions";
			this.lblInstallationOptions.Size = new System.Drawing.Size(118, 16);
			this.lblInstallationOptions.TabIndex = 14;
			this.lblInstallationOptions.Text = "Installation options";
			// 
			// chkLatestUnstable
			// 
			this.chkLatestUnstable.AutoSize = true;
			this.chkLatestUnstable.BackColor = System.Drawing.Color.Transparent;
			this.chkLatestUnstable.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.chkLatestUnstable.ForeColor = System.Drawing.SystemColors.ControlLightLight;
			this.chkLatestUnstable.Location = new System.Drawing.Point(403, 47);
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
			this.txtRelease.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.txtRelease.Location = new System.Drawing.Point(147, 49);
			this.txtRelease.Name = "txtRelease";
			this.txtRelease.Size = new System.Drawing.Size(71, 22);
			this.txtRelease.TabIndex = 8;
			// 
			// btnSelectFolder
			// 
			this.btnSelectFolder.BackColor = System.Drawing.SystemColors.GradientInactiveCaption;
			this.btnSelectFolder.BackgroundImage = global::AlienArenaInstaller.Properties.Resources.browsefolder;
			this.btnSelectFolder.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
			this.btnSelectFolder.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnSelectFolder.Location = new System.Drawing.Point(518, 85);
			this.btnSelectFolder.Name = "btnSelectFolder";
			this.btnSelectFolder.Size = new System.Drawing.Size(35, 25);
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
			this.txtInstallationFolder.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.txtInstallationFolder.Location = new System.Drawing.Point(147, 86);
			this.txtInstallationFolder.Name = "txtInstallationFolder";
			this.txtInstallationFolder.Size = new System.Drawing.Size(365, 22);
			this.txtInstallationFolder.TabIndex = 10;
			// 
			// toolStripProgressBar
			// 
			this.toolStripProgressBar.Name = "toolStripProgressBar";
			this.toolStripProgressBar.Size = new System.Drawing.Size(100, 16);
			// 
			// toolStripStatusLabel
			// 
			this.toolStripStatusLabel.Name = "toolStripStatusLabel";
			this.toolStripStatusLabel.Size = new System.Drawing.Size(0, 17);
			// 
			// Installer
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.BackgroundImage = global::AlienArenaInstaller.Properties.Resources.alienarena9;
			this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
			this.ClientSize = new System.Drawing.Size(914, 550);
			this.Controls.Add(this.panelInstallationOptions);
			this.Controls.Add(this.panelResolution);
			this.Controls.Add(this.statusStrip);
			this.Controls.Add(this.lblExplanation);
			this.Controls.Add(this.logo);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.Name = "Installer";
			this.Text = "Alien Arena Installer";
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
		private System.Windows.Forms.Panel logo;
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
	}
}

