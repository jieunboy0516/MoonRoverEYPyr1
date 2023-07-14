using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace csjsUDP
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Hello World!");

            while (true)
            {
                String data = Console.ReadLine();
                if (data.Equals("exit", StringComparison.OrdinalIgnoreCase)) break; //If the user types "exit" then quit the program

                SendData("127.0.0.1", 41181, data); //Send data to that host address, on that port, with this 'data' to be sent
                //Note the 41181 port is the same as the one we used in server.bind() in the Javascript file.

                System.Threading.Thread.Sleep(50); //Sleep for 50ms
            }



        }

        public static void SendData(string host, int destPort, string data)
        {
            IPAddress dest = Dns.GetHostAddresses(host)[0]; //Get the destination IP Address
            IPEndPoint ePoint = new IPEndPoint(dest, destPort);
            byte[] outBuffer = Encoding.ASCII.GetBytes(data); //Convert the data to a byte array
            Socket mySocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp); //Create a socket using the same protocols as in the Javascript file (Dgram and Udp)

            mySocket.SendTo(outBuffer, ePoint); //Send the data to the socket

            mySocket.Close(); //Socket use over, time to close it
        }


    }
}
