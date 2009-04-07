using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;
using System.Windows.Forms;



namespace FFDShowAPI
{
    public class FFDShowReceiver : System.Windows.Forms.Form
    {

        // The CopyData Constant for SendMessage
        public const Int32 WM_COPYDATA = 0x004A;

       
        [StructLayout(LayoutKind.Sequential)]
        internal struct COPYDATASTRUCT
        {
            internal UIntPtr dwData;
            internal uint cbData;
            internal IntPtr lpData;
        }

        public bool inUse = false;

        private String receivedString = null;
        private Int32 receivedType = 0;

        private Thread parentThread;


        public String ReceivedString
        {
            get
            {
                return receivedString;
            }
            set
            {
                receivedString = value;
            }
        }

        public Int32 ReceivedType
        {
            get
            {
                return receivedType;
            }
            set
            {
                receivedType = value;
            }
        }


        #region Constructors
        public FFDShowReceiver(Thread parentThread)
        {
            this.parentThread = parentThread;
        }
        #endregion Constructors

        // Receiver
        protected override void WndProc(ref Message m)
        {
            if (m.Msg == WM_COPYDATA)
            {
                try
                {
                    COPYDATASTRUCT cd = new COPYDATASTRUCT();
                    cd = (COPYDATASTRUCT)Marshal.PtrToStructure(m.LParam, typeof(COPYDATASTRUCT));

#if UNICODE
                    string returnedData = Marshal.PtrToStringUni(cd.lpData);
#else
                    string returnedData = Marshal.PtrToStringAnsi(cd.lpData);
#endif
                    receivedString = returnedData;
                    receivedType = (int)cd.dwData.ToUInt32();

                    /*if (receivedString != null)
                        Debug.WriteLine("Receiver got " + receivedType + " " + receivedString);
                    else
                        Debug.WriteLine("Receiver got " + receivedType + " NULL");
                    Debug.Flush();*/

                    if (parentThread != null && parentThread.ThreadState == System.Threading.ThreadState.WaitSleepJoin)
                        parentThread.Interrupt();
                    //resetEvent.Set();
                }
                catch (Exception)
                {
                    /*Debug.Write(e.StackTrace.ToString());
                    Debug.Flush();*/
                }
            }
            base.WndProc(ref m);
        }
    }
}
