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

        public void UpdateStatus(string message)
        {
            ListViewItem pItem = null;

            //good idea to clear the list after say 100 entries.

            pItem = StatusList.Items.Add(message);
            pItem.Selected = false;
            pItem.Focused = false;
        }
    }
}
