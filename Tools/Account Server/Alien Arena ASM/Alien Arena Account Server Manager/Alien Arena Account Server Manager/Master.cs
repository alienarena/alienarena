using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

using System.Net.Sockets;
using System.Net;
using System.Threading;
using System.Text;

namespace Alien_Arena_Account_Server_Manager
{
    public class masterServer
    {
        static public masterServer sServer = new masterServer();

        static UdpClient sListener;        
        static public bool runListener = false;
        static private ushort FrameTime = 0;

        //Note - it will make more sense eventually to use one list throughout the program.
        static ServerList Servers = new ServerList();

        public masterServer()
        {
            //nothing to do yet.
        }

        public class ServerList
        {
            public List<string> Ip;
            public List<ushort> Port;
            public List<ushort> LastHeartbeat;

            public ServerList()
            {
                Ip = new List<string>();
                Port = new List<ushort>();
                LastHeartbeat = new List<ushort>();
            }

            public void Add(string Ip, ushort Port, ushort LastHeartbeat)
            {
                this.Ip.Add(Ip);
                this.Port.Add(Port);
                this.LastHeartbeat.Add(LastHeartbeat);
            }

            public void Drop(int idx)
            {
                Ip.RemoveAt(idx);
                Port.RemoveAt(idx);
                LastHeartbeat.RemoveAt(idx);
            }

            public void Clear()
            {
                Ip.RemoveAll(AllStrings);
                Port.RemoveAll(AllUShorts);
                LastHeartbeat.RemoveAll(AllUShorts);
            }

            private static bool AllStrings(String s)
            {
                return true;
            }

            private static bool AllUShorts(ushort i)
            {
                return true;
            }
        }

        static void RunServerCheck()
        {
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                //Frametime has looped, so in this case update times of servers to be current.
                if (Servers.LastHeartbeat[i] > FrameTime)
                    Servers.LastHeartbeat[i] = FrameTime; 
                if(FrameTime - Servers.LastHeartbeat[i] > 2)
                {
                    //Never received a response from the ping sent
                    Servers.Drop(i);
                }
                else if(FrameTime - Servers.LastHeartbeat[i] > 1)
                {
                    IPAddress ip = IPAddress.Parse(Servers.Ip[i]);

                    string message = "ÿÿÿÿping";

                    IPEndPoint dest = new IPEndPoint(ip, Servers.Port[i]);

                    byte[] send_buffer = Encoding.Default.GetBytes(message);

                    //Send to client
                    try
                    {
                        sListener.Send(send_buffer, send_buffer.Length, dest);
                        ACCServer.sDialog.UpdateMasterStatus("Sending ping to " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".");
                    }
                    catch (Exception exc) { MessageBox.Show(exc.ToString()); }
                }
            }
        }

        static void SendServerListToClient(IPEndPoint dest)
        {
            string message = "ÿÿÿÿservers ";

            byte[] send_buffer = Encoding.Default.GetBytes(message);
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                IPAddress ip = IPAddress.Parse(Servers.Ip[i]);

                Array.Resize(ref send_buffer, send_buffer.Length + ip.GetAddressBytes().Length);
                Array.Copy(ip.GetAddressBytes(), 0, send_buffer, send_buffer.Length - ip.GetAddressBytes().Length, ip.GetAddressBytes().Length);

                byte[] port = BitConverter.GetBytes(Servers.Port[i]);
                Array.Reverse(port);

                Array.Resize(ref send_buffer, send_buffer.Length + port.Length);
                Array.Copy(port, 0, send_buffer, send_buffer.Length - port.Length, port.Length);
            }

            //Send to client
            try
            {
                sListener.Send(send_buffer, send_buffer.Length, dest);
                ACCServer.sDialog.UpdateMasterStatus("Sending server list to " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".");
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        static void HeartBeat(IPEndPoint dest)
        {
            for(int i = 0; i < Servers.Ip.Count; i++)
            {
                if (dest.Address.ToString() == Servers.Ip[i] && dest.Port == Servers.Port[i])
                {
                    //Matched, so update the hearbeat time of this server
                    Servers.LastHeartbeat[i] = FrameTime;

                    string message = "ÿÿÿÿack";

                    byte[] send_buffer = Encoding.Default.GetBytes(message);

                    //Send to client
                    try
                    {
                        sListener.Send(send_buffer, send_buffer.Length, dest);
                        ACCServer.sDialog.UpdateMasterStatus("Heartbeat from " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".");
                        return;
                    }
                    catch (Exception exc) { MessageBox.Show(exc.ToString()); }
                }
            }

            AddServerToList(dest);
        }

        static void Ack(IPEndPoint dest)
        {
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                if (dest.Address.ToString() == Servers.Ip[i] && dest.Port == Servers.Port[i])
                {
                    //Matched, so update the hearbeat time of this server
                    Servers.LastHeartbeat[i] = FrameTime;
                    ACCServer.sDialog.UpdateMasterStatus("Ack from " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".");
                }
            }
        }

        static void AddServerToList(IPEndPoint dest)
        {
            bool duplicate = false;

            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                if (dest.Address.ToString() == Servers.Ip[i] && dest.Port == Servers.Port[i])
                {
                    ACCServer.sDialog.UpdateMasterStatus("Ping from duplicate server " + dest.Address.ToString() + ":" + dest.Port.ToString() + " ignored!");
                    duplicate = true;
                }
            }

            if(!duplicate)
            {
                Servers.Add(dest.Address.ToString(), Convert.ToUInt16(dest.Port), FrameTime);
                ACCServer.sDialog.UpdateMasterStatus("Added " + dest.Address.ToString() + ":" + dest.Port.ToString() + " to list!");
            }
        }

        static void Shutdown(IPEndPoint dest)
        {
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                if (dest.Address.ToString() == Servers.Ip[i] && dest.Port == Servers.Port[i])
                {
                    ACCServer.sDialog.UpdateMasterStatus("Shutting down " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".");
                }
            }
        }

        static void ParseData(string message, IPEndPoint source)
        {            
            if (message.Contains("getservers") || message.Contains("query"))
            {                
                SendServerListToClient(source);
            }
            else if (message.Contains("ping"))
            {
                AddServerToList(source);
            }
            else if(message.Contains("ack"))
            {
                Ack(source);
            }
            else if(message.Contains("heartbeat"))
            {
                HeartBeat(source);
            }
            else if(message.Contains("shutdown"))
            {
                Shutdown(source);
            }
            else
               ACCServer.sDialog.UpdateMasterStatus("Unknown command from " + source.Address.ToString() + ":" + source.Port.ToString() + ".");
        }

        //Leave for now - it works safely across threads, but we can probably just use one list for this.
        public void GetServerList()
        {
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                Stats.Servers.Add(Servers.Ip[i], Servers.Port[i], "Server", "Map", 0);
            }
        }

        public void Listen()
        {
            sListener = new UdpClient(27900);
            sListener.Ttl = 100;
            IPEndPoint source = new IPEndPoint(0, 0);
            string received_data;
            byte[] receive_byte_array;
            try
            {
                while (runListener)
                {
                    receive_byte_array = sListener.Receive(ref source);
                    received_data = Encoding.Default.GetString(receive_byte_array, 0, receive_byte_array.Length);
                    if (received_data.Length > 1)
                        ParseData(received_data, source);

                    FrameTime = Convert.ToUInt16(DateTime.UtcNow.Minute);
                   // RunServerCheck();
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            try
            {
                sListener.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

    }
}