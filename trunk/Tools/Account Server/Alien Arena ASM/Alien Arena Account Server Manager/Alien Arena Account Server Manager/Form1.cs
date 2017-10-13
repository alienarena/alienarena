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
            netStuff.sServer.Start_Server();
        }
        
        private void StopServer_Click(object sender, EventArgs e)
        {
            netStuff.sServer.Stop_Server();
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

                pItem.EnsureVisible();
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

        public delegate void UpdateSTypeChartDelegate();

        public void UpdateSTypeChart()
        {
            try
            {
                this.ServerTypes.Series.Clear();

                Series series = this.ServerTypes.Series.Add("Servers");

                series.ChartType = SeriesChartType.Pie;
                series.Points.Add(Stats.DMServers);                
                series.Points.Add(Stats.CTFServers);
                series.Points.Add(Stats.TACServers);

                series.Points[0].AxisLabel = "Deathmatch";
                series.Points[1].AxisLabel = "CTF";
                series.Points[2].AxisLabel = "Tactical";
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdateServerTypeChart()
        {
            if (this.InvokeRequired == false)
            {
                UpdateSTypeChart();
            }
            else
            {
                UpdateSTypeChartDelegate updateSTypeChart = new UpdateSTypeChartDelegate(UpdateSTypeChart);
                this.Invoke(updateSTypeChart, new object[] { });
            }
        }

        public delegate void UpdateRankingsDelegate();

        public void UpdateRankings()
        {
            try
            {
                this.Rankings.Items.Clear();
                DBOperations.GenerateRankingsList();
                for (int i = 0; i < Stats.allPlayers.player.Count; i++)
                {
                    ListViewItem pItem = null;
                    pItem = Rankings.Items.Add((i+1).ToString());
                    pItem.Selected = false;
                    pItem.Focused = true;

                    pItem.SubItems.Add(Stats.allPlayers.player[i].Name);
                    pItem.SubItems.Add(Stats.allPlayers.player[i].StatPoints.ToString());
                    pItem.SubItems.Add(Stats.allPlayers.player[i].Status);
                }
                
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdateRankingList()
        {
            if (this.InvokeRequired == false)
            {
                UpdateRankings();
            }
            else
            {
                UpdateRankingsDelegate updateRankings = new UpdateRankingsDelegate(UpdateRankings);
                this.Invoke(updateRankings, new object[] { });
            }
        }

        public delegate void UpdateServersDelegate();

        public void UpdateServers()
        {
            try
            {
                this.ServerList.Items.Clear();

                for (int i = 0; i < Stats.Servers.Name.Count; i++)
                {
                    ListViewItem pItem = null;
                    pItem = ServerList.Items.Add(Stats.Servers.Name[i]);
                    pItem.Selected = false;
                    pItem.Focused = true;

                    pItem.SubItems.Add(Stats.Servers.Ip[i] + ":" + Stats.Servers.Port[i]);
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdateServerList()
        {
            if (this.InvokeRequired == false)
            {
                UpdateServers();
            }
            else
            {
                UpdateServersDelegate updateServers = new UpdateServersDelegate(UpdateServers);
                this.Invoke(updateServers, new object[] { });
            }
        }
    }
}
