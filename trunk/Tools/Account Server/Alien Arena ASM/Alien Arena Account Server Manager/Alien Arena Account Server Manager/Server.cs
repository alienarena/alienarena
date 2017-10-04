using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

using System.Data;
using System.Data.SqlClient;
using System.Net.Sockets;
using System.Net;
using System.Threading;
using System.Text;

namespace Alien_Arena_Account_Server_Manager
{
    public struct pProfile
    {
        public string Name;
        public string vString;
        public string Password;
    }

    public class playerList
    {
        public List<string> name;
        public List<int> time;

        public playerList()
        {
            name = new List<string>();
            time = new List<int>();
        }

        public void DropPlayer(string Name)
        {
            int idx = name.IndexOf(Name);

            name.RemoveAt(idx);
            time.RemoveAt(idx);
        }

        //this called only when a packet of "login" is received, and the player is validated
        public void AddPlayer(string Name)
        {
            //check if this player is already in the list
            int idx = name.IndexOf(Name);

            if (name.Exists(name => name == Name))
                return;

            name.Add(Name);

            //timestamp
            time.Add(DateTime.UtcNow.Hour);

            //check for expired players(those who never sent a logout)
            CheckPlayers();
        }

        //logout player
        public void RemovePlayer(string Name)
        {
            if (Name.Length < 1)
                return;

            if (name.Exists(name => name == Name))
                DropPlayer(Name);

            //DumpValidPlayersToFile(); //set db field to "inactive"
        }

        //check list for possible expired players(5 hour window)
        public void CheckPlayers()
        {
            int curTime = DateTime.UtcNow.Hour;

            for (int idx = 0; idx < name.Count; idx++)
            {
                if (time[idx] > curTime)
                {
                    curTime += 24;
                    if (curTime - time[idx] > 5)
                        DropPlayer(name[idx]);
                }

            }
        }
    }

    public class DBOperations
    {
        public static pProfile CheckPlayer(string Name)
        {
            pProfile Profile;

            Profile.Name = "Invalid";
            Profile.vString = "Invalid";
            Profile.Password = "Invalid";

            SqlConnection sqlConn = new SqlConnection("Server=MERCURY\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlDataReader rdr = null;

                SqlCommand cmd = new SqlCommand("SELECT Name, vString, Password FROM Players", sqlConn);
                rdr = cmd.ExecuteReader();
                while (rdr.Read())
                {
                    if (Name == rdr["Name"].ToString())
                    {
                        Profile.Name = rdr["Name"].ToString();
                        Profile.vString = rdr["vString"].ToString();
                        Profile.Password = rdr["Password"].ToString();
                    }
                }
                rdr.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            return Profile;
        }

        public static void AddProfile(string Name, string Password, string vString)
        {
            SqlConnection sqlConn = new SqlConnection("Server=MERCURY\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlCommand cmd = new SqlCommand("If NOT exists (select name from sysobjects where name = 'Players') CREATE TABLE Players(Name varchar(32), Password varchar(256), vString varchar(32), Status varchar(16));", sqlConn);

                cmd.ExecuteNonQuery();

                cmd.CommandText = "if NOT exists (SELECT * FROM Players where Name = @0) INSERT INTO Players(Name, Password, vString, Status) VALUES(@0, @1, @2, @3)";
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", Password));
                cmd.Parameters.Add(new SqlParameter("2", vString));
                cmd.Parameters.Add(new SqlParameter("3", "Active"));
                cmd.ExecuteNonQuery();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public static void ChangePlayerPassword(string Name, string NewPassword)
        {
            SqlConnection sqlConn = new SqlConnection("Server=MERCURY\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlCommand cmd = new SqlCommand("If NOT exists (select name from sysobjects where name = 'Players') CREATE TABLE Players(Name varchar(32), Password varchar(256), vString varchar(32), Status varchar(16));", sqlConn);

                cmd.ExecuteNonQuery();

                cmd.CommandText = "UPDATE Players SET Password = @1 WHERE Name = @0";
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", NewPassword));
                cmd.ExecuteNonQuery();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }
    }

    public class netStuff
    {
        public static netStuff sServer = new netStuff();
        static UdpClient sListener;
        static IPEndPoint source;

        public static playerList players = new playerList();

        public netStuff()
        {
            //nothing to do yet.
        }

        static string ObtainVStringForPlayer(string Name)
        {
            string vString = "";

            //look for existing account in DB
            pProfile Profile = DBOperations.CheckPlayer(Name);

            if (Profile.Name == "Invalid")
            {
                Random rnd = new Random();

                //create randomstring
                for (int i = 0; i < 32; i++)
                {
                    vString += Convert.ToChar(rnd.Next(0, 78) + 30);
                }
            }
            else
            {
                vString = Profile.vString;
            }

            return vString;
        }

        static bool ValidatePlayer(string Name, string Password, string vString)
        {
            //look for existing account in DB
            pProfile Profile = DBOperations.CheckPlayer(Name);

            //if no profile, create one and return true
            if (Profile.Name == "Invalid")
            {
                //add to database
                ACCServer.sDialog.UpdateStatus("Adding " + Name + " to database.");
                DBOperations.AddProfile(Name, Password, vString);
                return true;
            }
            else
            {                
                if (Password == Profile.Password)
                {
                    //matched!
                    ACCServer.sDialog.UpdateStatus("Found " + Name + " in database.");
                    return true;
                }
                else
                {
                    ACCServer.sDialog.UpdateStatus("Mismatched password for " + Name + " .");
                    return false;
                }
            }
        }

        static void SendValidationToClient()
        {
            Socket sending_socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);

            string message = "ÿÿÿÿvalidated";

            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sending_socket.SendTo(send_buffer, source);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            //Close this socket
            try
            {
                sending_socket.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        static void SendVStringToClient(string Name)
        {

            Socket sending_socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);

            string message = "ÿÿÿÿvstring ";

            message += ObtainVStringForPlayer(Name);
            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sending_socket.SendTo(send_buffer, source);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            //Close this socket
            try
            {
                sending_socket.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        static void ParseData(string message)
        {
            if (message.Contains("ÿÿÿÿrequestvstring"))
            {
                //It's a valid request string
                string[] sParams = message.Split('\\');

                //Check protocol
                if (sParams[2] != "1")
                {                    
                    ACCServer.sDialog.UpdateStatus("Wrong protocol " + sParams[2] +  " from " + source.Address.ToString() + ".");
                    return;
                }

                //Send verification string to client.
                ACCServer.sDialog.UpdateStatus("Sending verification string to " + sParams[4] + ".");
                SendVStringToClient(sParams[4]);
            }

            else if (message.Contains("ÿÿÿÿlogin"))
            {
                //Process a login request

                //Check protocol
                string[] sParams = message.Split('\\');

                if (sParams[2] != "1")
                {
                    ACCServer.sDialog.UpdateStatus("Wrong protocol " + sParams[2] + " from " + source.Address.ToString() + ".");
                    return;
                }

                if(ValidatePlayer(sParams[4], sParams[6], sParams[8]))
                {
                    ACCServer.sDialog.UpdateStatus("Adding " + sParams[4] + " to active player list.");
                    players.AddPlayer(sParams[4]);
                    SendValidationToClient();
                }
            }

            else if (message.Contains("ÿÿÿÿlogout"))
            {
                //Process a logout request

                //Check protocol
                string[] sParams = message.Split('\\');

                if (sParams[2] != "1")
                {
                    ACCServer.sDialog.UpdateStatus("Wrong protocol " + sParams[2] + " from " + source.Address.ToString() + ".");
                    return;
                }

                if (ValidatePlayer(sParams[4], sParams[6], sParams[8]))
                {
                    ACCServer.sDialog.UpdateStatus("Removing " + sParams[4] + " from active player list.");
                    players.RemovePlayer(sParams[4]);
                }

            }

            else if (message.Contains("ÿÿÿÿchangepw"))
            {
                //Process a password change request

                //Check protocol
                string[] sParams = message.Split('\\');

                if (sParams[2] != "1")
                {
                    ACCServer.sDialog.UpdateStatus("Wrong protocol " + sParams[2] + " from " + source.Address.ToString() + ".");
                    return;
                }

                if (sParams[6] == "password") //Setting from a new system for an existing player
                {
                    if (ValidatePlayer(sParams[4], sParams[8], sParams[10]))
                    {
                        ACCServer.sDialog.UpdateStatus("Setting password for " + sParams[4] + " .");
                        DBOperations.ChangePlayerPassword(sParams[4], sParams[8]);
                        SendValidationToClient();
                    }
                }
                else 
                {
                    if (ValidatePlayer(sParams[4], sParams[6], sParams[10]))
                    {
                        ACCServer.sDialog.UpdateStatus("Changing password for " + sParams[4] + " .");
                        DBOperations.ChangePlayerPassword(sParams[4], sParams[8]);
                        SendValidationToClient();
                    }
                }
            }
            else
            {
                //Unknown request
                ACCServer.sDialog.UpdateStatus("Unknown request!");
            }
        }

        static void OnUdpData(IAsyncResult result)
        {
            UdpClient socket = result.AsyncState as UdpClient;

            // points towards whoever had sent the message:
            source = new IPEndPoint(0, 0);

            // get the actual message and fill out the source:
            byte[] message = socket.EndReceive(result, ref source);

            string received_data = Encoding.Default.GetString(message, 0, message.Length);
            //MessageBox.Show(received_data);
            ParseData(received_data);
            // schedule the next receive operation once reading is done:
            socket.BeginReceive(new AsyncCallback(OnUdpData), socket);
        }

        public void OpenListener()
        {
            sListener = new UdpClient(27902);

            sListener.BeginReceive(new AsyncCallback(OnUdpData), sListener);

            ACCServer.sDialog.UpdateStatus("Listening...");
        }

        public void Close_Socket()
        {
            try
            {
                sListener.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
    }
}