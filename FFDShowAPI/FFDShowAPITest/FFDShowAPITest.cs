using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Text;
using System.Threading;
using System.Collections.Generic;
using FFDShowAPI;
using System.Drawing.Imaging;

namespace FFDShowAPI
{

    /// <summary>
    /// Summary description for Form1.
    /// </summary>
    public partial class FFDShowAPITest : System.Windows.Forms.Form
    {
        #region Variables and initialization
        private TabControl tabControl1;
        private TabPage tabPage1;
        private Button button6;
        private Button button4;
        private Button playButton;
        private Button FRewindButton;
        private Button FForwardButton;
        private Button button2;
        private Button button1;
        private TabPage tabPage2;
        private GroupBox groupBox1;
        private Label label1;
        private TextBox textBox1;
        private Label label3;
        private TrackBar trackBar2;
        private Label label2;
        private TrackBar trackBar1;
        private TabPage tabPage3;
        private SplitContainer splitContainer1;
        private Button button7;
        private SplitContainer splitContainer2;
        private GroupBox groupBox2;
        private CheckedListBox audioStreamlistBox;
        private GroupBox groupBox4;
        private CheckedListBox subtitleStreamlistBox;
        private SplitContainer splitContainer3;
        private SplitContainer splitContainer4;
        private GroupBox groupBox3;
        private ListBox listBox1;
        private Button button8;
        private GroupBox groupBox5;
        private Button ffdshowParamButton;
        private Label label5;
        private TextBox ffdshowParamValueBox;
        private Label label4;
        private TextBox ffdshowParamBox;

        public FFDShowAPITest Form1Ref;

        class NewCheckboxListItem
        {
            // define a text and
            // a tag value
            public string Text;
            public object Tag;

            // override ToString(); this
            // is what the checkbox control
            // displays as text
            public override string ToString()
            {
                return this.Text;
            }
        }

        public FFDShowAPITest()
        {
            //
            // Required for Windows Form Designer support
            //
            InitializeComponent();
            //
            // TODO: Add any constructor code after InitializeComponent call
            //
            Form1Ref = this;
        }
        #endregion Variables and initalization

        private void close_Click(object sender, System.EventArgs e)
        {
            this.Close();
        }

        private FFDShowAPI ffdshowAPI = null;

        /// <summary>
        /// Loads FFDShowAPI. First step to perform
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void FFDShow_click(object sender, EventArgs e)
        {
            /* FFDShowAPI can be initialiazed in several ways :
             * If no parameters are given, FFDShowAPI will take for the first instance found
             * Otherwise if known the filename of the media beeing played can be passed to narrow the search
             **/

            // Method 1 : pick the first one we find
            ffdshowAPI = new FFDShowAPI();

            // Method 2 (commented) : retrieve all the running instances and pick the one you want
            /*
            List<FFDShowAPI.FFDShowInstance> ffdshowInstances =  FFDShowAPI.getFFDShowInstances();
            for (int i=0;i<ffdshowInstances.Count;i++)
            {
                string fileName = ffdshowInstances[i].fileName; // Is this the one ?
                ffdshowAPI = new FFDShowAPI(ffdshowInstances[i].handle);
                break;
            }
            */

            // Method 3 (commented) retrieve the instance corresponding to the file
            /*
            string fileNameToLookFor = "sampleVideo.avi";
            ffdshowAPI = new FFDShowAPI(fileNameToLookFor, FFDShowAPI.FileNameMode.FileName);
            */

            // Check wether an existing instance is running (and corresponding to the constructor parameters, here any instance)
            bool isFFDShowActive = ffdshowAPI.checkFFDShowActive();


            if (isFFDShowActive)
                this.Text = "FFDShow has been found (" + ffdshowAPI.FFDShowAPIRemote + " handle used)";
            else
            {
                this.Text = "FFDShow has not been found (" + ffdshowAPI.FFDShowAPIRemote + " handle used)";
                return;
            }

            // Request a string parameter
            MessageBox.Show("Filename played : " + ffdshowAPI.getFileName());


            string streamsStr = "Audio streams :\n";
            try
            {
                SortedDictionary<int, FFDShowAPI.Stream> audioStreams = ffdshowAPI.AudioStreams;
                int currentStream = ffdshowAPI.AudioStream;
                foreach (KeyValuePair<int, FFDShowAPI.Stream> stream in audioStreams)
                {
                    streamsStr += stream.Key + " : " + stream.Value.name + "(" + stream.Value.languageName + ")";
                    if (currentStream == stream.Key)
                        streamsStr += " Active";
                    streamsStr += "\n";
                }
                MessageBox.Show(streamsStr);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error while retrieving audio streams : " + ex.Message + "\n" + ex.StackTrace.ToString());
            }

            streamsStr = "Subtitle streams :\n";
            try
            {
                SortedDictionary<int, FFDShowAPI.Stream> subtitleStreams = ffdshowAPI.SubtitleStreams;
                int currentStream = ffdshowAPI.SubtitleStream;
                foreach (KeyValuePair<int, FFDShowAPI.Stream> stream in subtitleStreams)
                {
                    streamsStr += stream.Key + " : " + stream.Value.name + "(" + stream.Value.languageName + ")";
                    if (currentStream == stream.Key)
                        streamsStr += " Active";
                    streamsStr += "\n";
                }
                MessageBox.Show(streamsStr);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error while retrieving subtitle streams : " + ex.Message + "\n" + ex.StackTrace.ToString());
            }

            string chaptersStr = "Chapters :\n";
            try
            {
                Dictionary<int, string> chaptersList = ffdshowAPI.ChaptersList;
                foreach (KeyValuePair<int, string> kvp in chaptersList)
                {
                    chaptersStr += kvp.Key + " : " + kvp.Value + "\n";
                }
                MessageBox.Show(chaptersStr);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error while retrieving chapters : " + ex.Message + "\n" + ex.StackTrace.ToString());
            }
        }

        /// <summary>
        /// Fast Forward through FFDShow
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void FForwardButton_Click(object sender, EventArgs e)
        {
            using (FFDShowAPI ffdshowAPI = new FFDShowAPI())
            {
                if (!ffdshowAPI.checkFFDShowActive()) return;

                int speed = ffdshowAPI.getFastForwardSpeed();
                switch (speed)
                {
                    case 0:
                        speed = 6; break;
                    case 6:
                        speed = 40; break;
                    case 40:
                        speed = 240; break;
                    default:
                        speed = 0; break;
                }
                ffdshowAPI.FastForward(speed);
            }
        }


        /// <summary>
        /// Play/Pause
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void playButton_Click(object sender, EventArgs e)
        {
            using (FFDShowAPI ffdshowAPI = new FFDShowAPI())
            {
                if (!ffdshowAPI.checkFFDShowActive()) return;
                if (ffdshowAPI.getFastForwardSpeed() != 0)
                {
                    ffdshowAPI.StopFastForward();
                    return;
                }
                if (ffdshowAPI.getState() == FFDShowAPI.PlayState.PlayState || ffdshowAPI.getState() == FFDShowAPI.PlayState.FastForwardRewind)
                    ffdshowAPI.pauseVideo();
                else
                    ffdshowAPI.startVideo();
            }
        }

        /// <summary>
        /// Rewind through FFDShow
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void FRewindButton_Click(object sender, EventArgs e)
        {
            using (FFDShowAPI ffdshowAPI = new FFDShowAPI())
            {
                if (!ffdshowAPI.checkFFDShowActive()) return;
                int speed = ffdshowAPI.getFastForwardSpeed();
                switch (speed)
                {
                    case 0:
                        speed = -6; break;
                    case -6:
                        speed = -40; break;
                    case -40:
                        speed = -240; break;
                    default:
                        speed = 0; break;
                }
                ffdshowAPI.FastForward(speed);
            }
        }

        /// <summary>
        /// List external subtitle files
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void subtitleFiles_Click(object sender, EventArgs e)
        {
            using (FFDShowAPI ffdshowAPI = new FFDShowAPI())
            {
                if (!ffdshowAPI.checkFFDShowActive()) return;
                string text = "Subtitle files :\nCurrent :" + ffdshowAPI.CurrentSubtitleFile + "\n";
                string[] subs = ffdshowAPI.SubtitleFiles;
                for (int i = 0; i < subs.Length; i++)
                {
                    text += i + " : " + subs[i] + "\n";
                }
                MessageBox.Show(text);
            }
        }

        /// <summary>
        /// Retrieves audio and subtitle streams through FFDShow
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void getFFDShowStreams_Click(object sender, EventArgs e)
        {
            this.audioStreamlistBox.ItemCheck -= new System.Windows.Forms.ItemCheckEventHandler(this.audioStreamlistBox_ItemCheck);
            this.subtitleStreamlistBox.ItemCheck -= new System.Windows.Forms.ItemCheckEventHandler(this.subtitleStreamlistBox_ItemCheck);
            audioStreamlistBox.Items.Clear();
            subtitleStreamlistBox.Items.Clear();
            using (FFDShowAPI ffdshowAPI = new FFDShowAPI())
            {
                if (!ffdshowAPI.checkFFDShowActive()) return;
                SortedDictionary<int, FFDShowAPI.Stream> audioStreams = ffdshowAPI.AudioStreams;
                SortedDictionary<int, FFDShowAPI.Stream> subtitleStreams = ffdshowAPI.SubtitleStreams;

                int currentAudioStream = ffdshowAPI.AudioStream;
                int currentSubtitleStream = ffdshowAPI.SubtitleStream;

                foreach (KeyValuePair<int,FFDShowAPI.Stream> stream in audioStreams)
                {
                    NewCheckboxListItem chk = new NewCheckboxListItem();
                    chk.Text = stream.Value.name + "(" + stream.Value.languageName + ")";
                    chk.Tag = stream.Key;
                    audioStreamlistBox.Items.Add(chk);
                    if (stream.Key == currentAudioStream)
                    {
                        audioStreamlistBox.SetItemChecked(audioStreamlistBox.Items.Count - 1, true);
                    }
                }

                foreach (KeyValuePair<int, FFDShowAPI.Stream> stream in subtitleStreams)
                {
                    NewCheckboxListItem chk = new NewCheckboxListItem();
                    chk.Text = stream.Value.name + "(" + stream.Value.languageName + ")";
                    chk.Tag = stream.Key;
                    subtitleStreamlistBox.Items.Add(chk);
                    if (stream.Key == currentSubtitleStream)
                    {
                        subtitleStreamlistBox.SetItemChecked(subtitleStreamlistBox.Items.Count-1, true);
                    }
                }
                subtitleStreamlistBox.Refresh();
                audioStreamlistBox.Refresh();
                tabControl1.SelectedTab = tabPage3;
                this.audioStreamlistBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.audioStreamlistBox_ItemCheck);
                this.subtitleStreamlistBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.subtitleStreamlistBox_ItemCheck);
            }
        }

        private int OSDX = 0;
        private int OSDY = 10;

        /// <summary>
        /// Sets the FFDShow OSD position
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OSDX_ValueChanged(object sender, EventArgs e)
        {
            if (ffdshowAPI == null)
                ffdshowAPI = new FFDShowAPI();
            if (!ffdshowAPI.checkFFDShowActive()) return;
            OSDX = trackBar1.Value;
            ffdshowAPI.setOSDPosition(OSDX, OSDY);
        }

        /// <summary>
        /// Sends a string to FFDShow OSD to be displayed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OSD_TextChanged(object sender, EventArgs e)
        {
            if (ffdshowAPI == null)
                ffdshowAPI = new FFDShowAPI();
            if (!ffdshowAPI.checkFFDShowActive()) return;
            ffdshowAPI.displayOSDMessage(textBox1.Text);
        }

        private void OSDY_ValueChanged(object sender, EventArgs e)
        {
            if (ffdshowAPI == null)
                ffdshowAPI = new FFDShowAPI();
            if (!ffdshowAPI.checkFFDShowActive()) return;

            OSDX = trackBar2.Value;
            ffdshowAPI.setOSDPosition(OSDX, OSDY);
        }

        /// <summary>
        /// Switch of audio stream
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void audioStreamlistBox_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            NewCheckboxListItem item =  (NewCheckboxListItem) audioStreamlistBox.SelectedItem;
            int streamNb = (int)item.Tag;
            using (FFDShowAPI ffdshowAPI = new FFDShowAPI())
            {
                if (!ffdshowAPI.checkFFDShowActive()) return;
                SortedDictionary<int, FFDShowAPI.Stream> audioStreams = ffdshowAPI.AudioStreams;
                ffdshowAPI.AudioStream = streamNb;
                getFFDShowStreams_Click(null, null);
            }
        }

        /// <summary>
        /// Switch of subtitle stream
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void subtitleStreamlistBox_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            NewCheckboxListItem item = (NewCheckboxListItem)subtitleStreamlistBox.SelectedItem;
            int streamNb = (int)item.Tag;
            using (FFDShowAPI ffdshowAPI = new FFDShowAPI())
            {
                if (!ffdshowAPI.checkFFDShowActive()) return;
                SortedDictionary<int, FFDShowAPI.Stream> subtitleStreams = ffdshowAPI.SubtitleStreams;
                ffdshowAPI.SubtitleStream = streamNb;
                getFFDShowStreams_Click(null, null);
            }
        }


        /// <summary>
        /// Sends an integer parameter to FFDShow (advanced users)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ffdshowParamButton_Click(object sender, EventArgs e)
        {
            try
            {
                using (FFDShowAPI ffdshowAPI = new FFDShowAPI())
                {
                    if (!ffdshowAPI.checkFFDShowActive()) return;
                    ffdshowAPI.setIntParam((FFDShowConstants.FFDShowDataId)int.Parse(ffdshowParamBox.Text),
                    int.Parse(ffdshowParamValueBox.Text));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.StackTrace);
            }
        }
    }
}
