namespace Alien_Arena_Account_Server_Manager
{
    partial class ACCServer
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
            this.StatusList = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.StartServer = new System.Windows.Forms.Button();
            this.StopServer = new System.Windows.Forms.Button();
            this.PlayerList = new System.Windows.Forms.ListView();
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.SuspendLayout();
            // 
            // StatusList
            // 
            this.StatusList.AutoArrange = false;
            this.StatusList.BackColor = System.Drawing.SystemColors.MenuText;
            this.StatusList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1});
            this.StatusList.ForeColor = System.Drawing.SystemColors.Info;
            this.StatusList.Location = new System.Drawing.Point(12, 12);
            this.StatusList.Name = "StatusList";
            this.StatusList.Size = new System.Drawing.Size(255, 403);
            this.StatusList.TabIndex = 0;
            this.StatusList.UseCompatibleStateImageBehavior = false;
            this.StatusList.View = System.Windows.Forms.View.Details;
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = "Status";
            this.columnHeader1.Width = 245;
            // 
            // StartServer
            // 
            this.StartServer.Location = new System.Drawing.Point(12, 421);
            this.StartServer.Name = "StartServer";
            this.StartServer.Size = new System.Drawing.Size(75, 23);
            this.StartServer.TabIndex = 1;
            this.StartServer.Text = "Start Server";
            this.StartServer.UseVisualStyleBackColor = true;
            this.StartServer.Click += new System.EventHandler(this.button1_Click);
            // 
            // StopServer
            // 
            this.StopServer.Location = new System.Drawing.Point(93, 421);
            this.StopServer.Name = "StopServer";
            this.StopServer.Size = new System.Drawing.Size(75, 23);
            this.StopServer.TabIndex = 2;
            this.StopServer.Text = "Stop Server";
            this.StopServer.UseVisualStyleBackColor = true;
            this.StopServer.Click += new System.EventHandler(this.StopServer_Click);
            // 
            // PlayerList
            // 
            this.PlayerList.BackColor = System.Drawing.SystemColors.InfoText;
            this.PlayerList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader2});
            this.PlayerList.ForeColor = System.Drawing.SystemColors.Info;
            this.PlayerList.Location = new System.Drawing.Point(273, 12);
            this.PlayerList.Name = "PlayerList";
            this.PlayerList.Size = new System.Drawing.Size(168, 403);
            this.PlayerList.TabIndex = 3;
            this.PlayerList.UseCompatibleStateImageBehavior = false;
            this.PlayerList.View = System.Windows.Forms.View.Details;
            // 
            // columnHeader2
            // 
            this.columnHeader2.Text = "Active Players";
            this.columnHeader2.Width = 154;
            // 
            // ACCServer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(453, 456);
            this.Controls.Add(this.PlayerList);
            this.Controls.Add(this.StopServer);
            this.Controls.Add(this.StartServer);
            this.Controls.Add(this.StatusList);
            this.Name = "ACCServer";
            this.Text = "Account Server";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListView StatusList;
        private System.Windows.Forms.Button StartServer;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.Button StopServer;
        private System.Windows.Forms.ListView PlayerList;
        private System.Windows.Forms.ColumnHeader columnHeader2;
    }
}

