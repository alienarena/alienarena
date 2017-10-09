using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace Alien_Arena_Account_Server_Manager
{
    public partial class ACCServer : Form
    {
        public static ACCServer sDialog;

        public ACCServer()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            netStuff.sServer.OpenListener();
        }
        
        private void StopServer_Click(object sender, EventArgs e)
        {
            netStuff.sServer.Close_Socket();
            PlayerList.Items.Clear();
            netStuff.players.Clear();
            UpdateStatus("stopped...");
        }

        public void UpdateStatus(string message)
        {
            ListViewItem pItem = null;

            //good idea to clear the list after say 100 entries.
            if (StatusList.Items.Count > 100)
                StatusList.Items.Clear();

            pItem = StatusList.Items.Add(message);
            pItem.Selected = false;
            pItem.Focused = false;
        }

        public void UpdatePlayerList()
        {
            PlayerList.Items.Clear();

            for(int i = 0; i < netStuff.players.name.Count; i++)
            {
                ListViewItem pItem = null;
                pItem = PlayerList.Items.Add(netStuff.players.name[i]);
                pItem.Selected = false;
                pItem.Focused = false;
            }
        }

        public void UpdateServerChart()
        {
            this.servercount.Series.Clear();

            Series series = this.servercount.Series.Add("Servers");

            //track last 10 polls.
            for (int i = Stats.ServerCount.Count-1; i >= 0; i--)
            {   
                series.Points.Add(Stats.ServerCount[i]);
            }
        }
        public void UpdatePlayerChart()
        {
            this.playercount.Series.Clear();

            Series series = this.playercount.Series.Add("All Clients");

            //track last 10 polls.
            for (int i = Stats.PlayerCount.Count - 1; i >= 0; i--)
            {
                series.Points.Add(Stats.PlayerCount[i]);
            }

            series = this.playercount.Series.Add("Validated");

            //track last 10 polls.
            for (int i = Stats.ValidatedPlayerCount.Count - 1; i >= 0; i--)
            {
                series.Points.Add(Stats.ValidatedPlayerCount[i]);
            }
        }
    }
}
