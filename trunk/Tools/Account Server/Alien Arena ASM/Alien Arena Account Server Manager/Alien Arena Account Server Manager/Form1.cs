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

        public delegate void UpdateStatusDelegate(string message);

        public void UpdateStatusList(string message)
        {
            try
            {
                ListViewItem pItem = null;

                //good idea to clear the list after say 1000 entries.
                if (StatusList.Items.Count > 1000)
                    StatusList.Items.Clear();

                pItem = StatusList.Items.Add(message);
                pItem.Selected = false;
                pItem.Focused = false;
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void UpdateStatus(string message)
        {
            if (this.InvokeRequired == false)
            {
                UpdateStatusList(message);
            }
            else
            {
                UpdateStatusDelegate updateStatus = new UpdateStatusDelegate(UpdateStatusList);
                this.Invoke(updateStatus, new object[] { message });
            }

        }

        public delegate void UpdateVPlayerListDelegate();

        public void UpdateVPlayerList()
        {
            try
            {
                PlayerList.Items.Clear();

                for (int i = 0; i < netStuff.players.name.Count; i++)
                {
                    ListViewItem pItem = null;
                    pItem = PlayerList.Items.Add(netStuff.players.name[i]);
                    pItem.Selected = false;
                    pItem.Focused = true;
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void UpdatePlayerList()
        {
            if(this.InvokeRequired == false)
            {
                UpdateVPlayerList();
            }
            else
            {
                UpdateVPlayerListDelegate updatePlayerList = new UpdateVPlayerListDelegate(UpdateVPlayerList);
                this.Invoke(updatePlayerList, new object[] {});
            }
        }

        public delegate void UpdateSChartDelegate();

        public void UpdateSChart()
        {
            try
            {
                this.servercount.Series.Clear();

                Series series = this.servercount.Series.Add("Servers");

                for (int i = Stats.ServerCount.Count - 1; i >= 0; i--)
                {
                    series.Points.Add(Stats.ServerCount[i]);
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdateServerChart()
        {
            if (this.InvokeRequired == false)
            {
                UpdateSChart();
            }
            else
            {
                UpdateSChartDelegate updateSChart = new UpdateSChartDelegate(UpdateSChart);
                this.Invoke(updateSChart, new object[] {});
            }
        }

        public delegate void UpdatePChartDelegate();

        public void UpdatePChart()
        {
            try
            {
                this.playercount.Series.Clear();

                Series series = this.playercount.Series.Add("All Clients");

                for (int i = Stats.PlayerCount.Count - 1; i >= 0; i--)
                {
                    series.Points.Add(Stats.PlayerCount[i]);
                }

                Series vseries = this.playercount.Series.Add("Validated");

                for (int i = Stats.ValidatedPlayerCount.Count - 1; i >= 0; i--)
                {
                    vseries.Points.Add(Stats.ValidatedPlayerCount[i]);
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdatePlayerChart()
        {
            if (this.InvokeRequired == false)
            {
                UpdatePChart();
            }
            else
            {
                UpdatePChartDelegate updatePChart = new UpdatePChartDelegate(UpdatePChart);
                this.Invoke(updatePChart, new object[] { });
            }
        }
    }
}
