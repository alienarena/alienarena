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
using System.ComponentModel;
using System.IO;

namespace Alien_Arena_Account_Server_Manager
{
    public struct pProfile
    {
        public string Name;
        public string Location;
        public string Password;
        public double StatPoints;
        public double TotalFragRate;
        public double TotalTime;
        public string Status;
    }

    public class playerList
    {
        public List<string> name;
        public List<int> score;
        public List<int> frags;
        public List<int> hour;
        public List<int> minutes;

        public playerList()
        {
            name = new List<string>();
            score = new List<int>();
            frags = new List<int>();
            hour = new List<int>();
            minutes = new List<int>();
        }

        public void DropPlayer(string Name)
        {
            int idx = name.IndexOf(Name);

            name.RemoveAt(idx);
            score.RemoveAt(idx);
            frags.RemoveAt(idx);
            hour.RemoveAt(idx);
            minutes.RemoveAt(idx);

            //we've removed a player, update the listview
            ACCServer.sDialog.UpdatePlayerList();
        }

        //this called only when a packet of "login" is received, and the player is validated
        public void AddPlayer(string Name)
        {
            //check if this player is already in the list
            int idx = name.IndexOf(Name);

            if (name.Exists(name => name == Name))
                return;

            name.Add(Name);
            score.Add(0); //no score needed for this list
            frags.Add(0);

            //timestamp
            hour.Add(DateTime.UtcNow.Hour);
            minutes.Add(DateTime.UtcNow.Minute);

            //we've added a player, update the listview
            ACCServer.sDialog.UpdatePlayerList();

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

            //set db field to "inactive"
            DBOperations.SetPlayerStatus(Name, "Inactive");
        }

        public void AddPlayerInfo(string Name, int Score, int Frags, int Hour, int Minutes)
        {
            //we need to actually use insert and put these in a sorted order by fragrates
            int insertPos = 0;

            //check if this player is already in the list
            int idx = name.IndexOf(Name);

            if (name.Exists(name => name == Name))
                return;

            for(idx = 0; idx < name.Count; idx++)
            {
                if ((float)Frags / ((float)Hour * 60.0f + (float)Minutes) >= (float)frags[idx] / ((float)hour[idx] * 60.0f + (float)minutes[idx]))
                    insertPos = idx;
            }

            name.Insert(insertPos, Name);
            score.Insert(insertPos, Score);
            frags.Insert(insertPos, Frags); 
            hour.Insert(insertPos, Hour);
            minutes.Insert(insertPos, Minutes);
        }

        //check list for possible expired players(5 hour window)
        public void CheckPlayers()
        {
            int curTime = DateTime.UtcNow.Hour;

            for (int idx = 0; idx < name.Count; idx++)
            {
                if (hour[idx] > curTime)
                {
                    curTime += 24;
                    if (curTime - hour[idx] > 5)
                        DropPlayer(name[idx]);
                }
            }
        }

        public int GetPlayerIndex(string Name)
        {
            for (int idx = 0; idx < name.Count; idx++)
            {
                if (Name == name[idx])
                {
                    return idx;
                }
            }

            return -1;
        }

        //clear the list completely
        public void Clear()
        {
            name.RemoveAll(AllStrings);
            score.RemoveAll(AllInts);
            frags.RemoveAll(AllInts);
            hour.RemoveAll(AllInts);
            minutes.RemoveAll(AllInts);
        }

        private static bool AllStrings(String s)
        {
            return true;
        }

        private static bool AllInts(int i)
        {
            return true;
        }
    }

    public class DBOperations
    {
        public static pProfile CheckPlayer(string Name)
        {
            pProfile Profile;

            Profile.Name = "Invalid";
            Profile.Location = "Invalid";
            Profile.Password = "Invalid";
            Profile.StatPoints = 0.0f;
            Profile.TotalFragRate = 0.0f;
            Profile.TotalTime = 0.0f;
            Profile.Status = "Inactive";

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

                SqlCommand cmd = new SqlCommand("SELECT Name, Password, Points, TotalFragRate, TotalTime, Location, Status FROM Players WHERE Name = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Name));
                rdr = cmd.ExecuteReader();
                while (rdr.Read())
                {
                    if (Name == rdr["Name"].ToString())
                    {
                        Profile.Name = rdr["Name"].ToString();
                        Profile.Location = rdr["Location"].ToString();
                        Profile.Password = rdr["Password"].ToString();
                        Profile.StatPoints = Convert.ToDouble(rdr["Points"]);
                        Profile.TotalFragRate = Convert.ToDouble(rdr["TotalFragRate"]);
                        Profile.TotalTime = Convert.ToDouble(rdr["TotalTime"]);
                        Profile.Status = rdr["Status"].ToString();
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

        public static void AddProfile(string Name, string Password, string Location)
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
                SqlCommand cmd = new SqlCommand("If NOT exists (select name from sysobjects where name = 'Players') CREATE TABLE Players(Name varchar(32), Password varchar(256), Points float, TotalFragRate float, TotalTime int, Location varchar(32), Status varchar(16));", sqlConn);

                cmd.ExecuteNonQuery();

                cmd.CommandText = "if NOT exists (SELECT * FROM Players WHERE Name = @0) INSERT INTO Players(Name, Password, Points, TotalFragRate, TotalTime, Location, Status) VALUES(@0, @1, 0.0, 0.0, 0, @2, @3)";
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", Password));
                cmd.Parameters.Add(new SqlParameter("2", Location));
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

        public static void SetPlayerStatus(string Name, string Status)
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
                SqlCommand cmd = new SqlCommand("UPDATE Players SET Status = @1 WHERE Name = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", Status));
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
                SqlCommand cmd = new SqlCommand("UPDATE Players SET Password = @1 WHERE Name = @0", sqlConn);
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

        public static void UpdatePlayer(string Name, string Points, string TotalFragRate, string TotalTime )
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
                SqlCommand cmd = new SqlCommand("UPDATE Players SET Points = @1, TotalFragRate = @2, TotalTime = @3 WHERE Name = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", Math.Round(Convert.ToDouble(Points), 3)));
                cmd.Parameters.Add(new SqlParameter("2", Math.Round(Convert.ToDouble(TotalFragRate), 3)));
                cmd.Parameters.Add(new SqlParameter("3", Convert.ToInt32(TotalTime)));
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

        public static void GenerateStatsFile()
        {
            pProfile Profile;

            Profile.Name = "Invalid";
            Profile.Location = "Invalid";
            Profile.Password = "Invalid";
            Profile.StatPoints = 0.0f;
            Profile.TotalFragRate = 0.0f;
            Profile.TotalTime = 0.0f;
            Profile.Status = "Inactive";

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
                int total = 0;
                SqlDataReader rdr = null;

                SqlCommand cmd = new SqlCommand("SELECT * FROM Players ORDER BY Points DESC", sqlConn);

                StreamWriter file = new System.IO.StreamWriter(@"playerrank.db");

                rdr = cmd.ExecuteReader();
                while (rdr.Read() && total < 1000)
                {
                    file.WriteLine(rdr["Name"].ToString());
                    file.WriteLine("127.0.0.1");
                    file.WriteLine(rdr["Points"].ToString());
                    //current frags
                    file.WriteLine("0");
                    //total fragrate
                    file.WriteLine(rdr["TotalFragRate"].ToString());
                    //current time
                    file.WriteLine("0");
                    file.WriteLine(rdr["TotalTime"].ToString());
                    //next two not needed any longer(server ip and poll number)
                    file.WriteLine("0");
                    file.WriteLine("0");

                    total++;
                }
                rdr.Close();
                file.Close();
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
        static Thread RunStats;
        static Thread UploadStats;
        static Thread RunListener;
        static bool runListener = false;

        public static playerList players = new playerList();

        public netStuff()
        {
            //nothing to do yet.
        }

        static string ObtainVStringForPlayer(string Name)
        {
            string vString = "";

            Random rnd = new Random();

            //create randomstring
            for (int i = 0; i < 32; i++)
            {
                vString += Convert.ToChar(rnd.Next(0, 78) + 30);
            }
           
            return vString;
        }

        static bool ValidatePlayer(string Name, string Password, string Location)
        {
            //look for existing account in DB
            pProfile Profile = DBOperations.CheckPlayer(Name);

            //if no profile, create one and return true
            if (Profile.Name == "Invalid")
            {
                //add to database
                ACCServer.sDialog.UpdateStatus("Adding " + Name + " to database.");
                DBOperations.AddProfile(Name, Password, Location);
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

        static void SendValidationToClient(IPEndPoint dest)
        {
            string message = "ÿÿÿÿvalidated";

            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sListener.Send(send_buffer, send_buffer.Length, dest);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }


        static void SendVStringToClient(string Name, IPEndPoint dest)
        {
            string message = "ÿÿÿÿvstring ";

            message += ObtainVStringForPlayer(Name);
            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sListener.Send(send_buffer, send_buffer.Length, dest);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        static void ParseData(string message, IPEndPoint source)
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
                ACCServer.sDialog.UpdateStatus("Sending verification string to " + sParams[4] + " at " + source.Address.ToString() + ":" + source.Port.ToString() + ".");
                SendVStringToClient(sParams[4], source);
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

                if(ValidatePlayer(sParams[4], sParams[6], source.Address.ToString()))
                {
                    ACCServer.sDialog.UpdateStatus("Adding " + sParams[4] + " to active player list.");
                    players.AddPlayer(sParams[4]);
                    SendValidationToClient(source);
                    DBOperations.SetPlayerStatus(sParams[4], "Active");
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

                if (ValidatePlayer(sParams[4], sParams[6], source.Address.ToString()))
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
                    if (ValidatePlayer(sParams[4], sParams[8], source.Address.ToString()))
                    {
                        ACCServer.sDialog.UpdateStatus("Setting password for " + sParams[4] + " .");
                        DBOperations.ChangePlayerPassword(sParams[4], sParams[8]);
                        SendValidationToClient(source);
                    }
                }
                else 
                {
                    if (ValidatePlayer(sParams[4], sParams[6], source.Address.ToString()))
                    {
                        ACCServer.sDialog.UpdateStatus("Changing password for " + sParams[4] + " .");
                        DBOperations.ChangePlayerPassword(sParams[4], sParams[8]);
                        SendValidationToClient(source);
                    }
                }
            }            
            else
            {
                //Unknown request
                ACCServer.sDialog.UpdateStatus("Unknown request! " + message);
            }
        }
               
        public void RequestServerList()
        {
            IPEndPoint Master = new IPEndPoint(IPAddress.Parse("69.243.97.80"), 27900);
            //master 2 149.210.138.19
            //master 1 69.243.97.80

            Socket sending_socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            
            string message = "query";

            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sending_socket.SendTo(send_buffer, Master);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            byte[] bytes = new byte[1024];
            try
            {
                sending_socket.Receive(bytes);

                int start = 12;
                int result = bytes.Length - 12;
                while (result > 0)
                {
                    //read 32 bit IP address (network byte order)
                    byte[] ip = bytes.Skip(start).Take(4).ToArray();
                    IPAddress sIP = new IPAddress(ip);

                    if (sIP.ToString() == "0.0.0.0")
                        break;

                    start += 4;

                    byte[] port = bytes.Skip(start).Take(2).ToArray();
                    Array.Reverse(port);
                    ushort sPort = BitConverter.ToUInt16(port, 0);
                    start += 2;

                    result -= 6; //6 bytes per server entry

                    //Add to list
                    Stats.Servers.Add(sIP.ToString(), sPort, "Server", "Map");
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            //Close this socket
            try
            {
                sending_socket.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void GetServerInfo(string Ip, ushort Port, int sNum)
        {
            IPEndPoint Server = new IPEndPoint(IPAddress.Parse(Ip), Port);

            Socket sending_socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            sending_socket.Ttl = 60;

            string message = "\xFF\xFF\xFF\xFFstatus\n";

            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sending_socket.SendTo(send_buffer, Server);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            byte[] bytes = new byte[1024];
            try
            {
                sending_socket.ReceiveTimeout = 3;
                sending_socket.Receive(bytes);

                message = Encoding.Default.GetString(bytes, 0, bytes.Length);

                //MessageBox.Show(message);

                string[] sParams = message.Split('\\');

                for (int i = 0; i < sParams.Length; i++)
                {
                    //after "mods" comes the large space delimited piece with player info
                    //MessageBox.Show(sParams[i]);                   

                    if(i != 0)
                    {
                        if (sParams[i - 1] == "hostname")
                            Stats.Servers.Name[sNum] = sParams[i];
                        if(sParams[i - 1] == "mapname")
                            Stats.Servers.Map[sNum] = sParams[i];

                        if (sParams[i - 1] == "mods" && sParams[i].Length > 0)
                        {
                            if (sParams[i].Contains("ctf") || sParams[i].Contains("g_tactical"))
                                Stats.Servers.Name[sNum] = "Skip";

                            string[] sPlayers = sParams[i].Split('\n');
                            for(int j = 0; j < sPlayers.Length; j++)
                            {
                                if (sPlayers[j].Length > 0 && j > 0)
                                {
                                    string[] sPlayer = sPlayers[j].Split(' ');
                                    if (sPlayer.Length > 2)
                                    {
                                        string Name = sPlayer[2].Trim('"');
                                        //Get index of this player in active, signed in player list
                                        Stats.TotalPlayers++;
                                        //If this player is not logged in, forget them
                                        int idx = players.GetPlayerIndex(Name);
                                        if (idx != -1)
                                        {
                                            int Score = Convert.ToInt32(sPlayer[0]);
                                            int Hours = DateTime.UtcNow.Hour - players.hour[idx];
                                            int Minutes = DateTime.UtcNow.Minute - players.minutes[idx];
                                            if (Hours < 0)
                                            {
                                                Hours += 24;
                                                Minutes = 60 - Minutes;
                                            }
                                            int Frags = players.frags[idx] + (Score - players.score[idx] > 0? Score - players.score[idx]:0);
                                            //check score vs what's already in the registered list to get frags to add to total
                                            Stats.players.AddPlayerInfo(Name, Score, Frags, Hours, Minutes);
                                            //Then update the registered list with the updated score and frag total.
                                            players.frags[idx] = Frags;
                                            players.score[idx] = Score;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception exc) {  }

            //Close this socket
            try
            {
                sending_socket.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void Listen()
        {
            bool done = false;
            sListener = new UdpClient(27902);
            sListener.Ttl = 100;
            IPEndPoint source = new IPEndPoint(0, 0);
            string received_data;
            byte[] receive_byte_array;
            try
            {
                while (!done)
                {
                    receive_byte_array = sListener.Receive(ref source);
                    received_data = Encoding.Default.GetString(receive_byte_array, 0, receive_byte_array.Length);
                    if (received_data.Length > 4)
                        ParseData(received_data, source);
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            try
            {
                sListener.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void Start_Server()
        {
            ACCServer.sDialog.UpdateStatus("Listening...");

            //Start a thread for the listener.
            runListener = true;
            RunListener = new Thread(new ThreadStart(Listen));
            RunListener.Start();

            //Start new thread for stats collection.
            Stats.getStats = true;
            RunStats = new Thread(new ThreadStart(Stats.StatsGen));
            RunStats.Start();

            //Start new thread for stats upload.
            Stats.uploadStats = true;
            UploadStats = new Thread(new ThreadStart(Stats.UploadStats));
            UploadStats.Start();
        }

        public void Stop_Server()
        {
            try
            {
                runListener = false;
                Stats.getStats = false;
                Stats.uploadStats = false;                       
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
    }
}