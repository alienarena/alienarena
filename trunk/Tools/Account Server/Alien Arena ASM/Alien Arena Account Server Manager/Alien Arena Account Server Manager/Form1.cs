using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

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
    }
}
