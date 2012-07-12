namespace FFDShowAPI
{
    partial class FFDShowAPITest
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.button6 = new System.Windows.Forms.Button();
            this.button4 = new System.Windows.Forms.Button();
            this.playButton = new System.Windows.Forms.Button();
            this.FRewindButton = new System.Windows.Forms.Button();
            this.FForwardButton = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.splitContainer2 = new System.Windows.Forms.SplitContainer();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.audioStreamlistBox = new System.Windows.Forms.CheckedListBox();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.subtitleStreamlistBox = new System.Windows.Forms.CheckedListBox();
            this.button7 = new System.Windows.Forms.Button();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.ffdshowParamButton = new System.Windows.Forms.Button();
            this.label5 = new System.Windows.Forms.Label();
            this.ffdshowParamValueBox = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.ffdshowParamBox = new System.Windows.Forms.TextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label3 = new System.Windows.Forms.Label();
            this.trackBar2 = new System.Windows.Forms.TrackBar();
            this.label2 = new System.Windows.Forms.Label();
            this.trackBar1 = new System.Windows.Forms.TrackBar();
            this.label1 = new System.Windows.Forms.Label();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.splitContainer3 = new System.Windows.Forms.SplitContainer();
            this.splitContainer4 = new System.Windows.Forms.SplitContainer();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.listBox1 = new System.Windows.Forms.ListBox();
            this.button8 = new System.Windows.Forms.Button();
            this.tabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.tabPage3.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.splitContainer2.Panel1.SuspendLayout();
            this.splitContainer2.Panel2.SuspendLayout();
            this.splitContainer2.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
            this.splitContainer3.Panel1.SuspendLayout();
            this.splitContainer3.Panel2.SuspendLayout();
            this.splitContainer3.SuspendLayout();
            this.splitContainer4.Panel1.SuspendLayout();
            this.splitContainer4.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.SuspendLayout();
            //
            // tabControl1
            //
            this.tabControl1.Controls.Add(this.tabPage1);
            this.tabControl1.Controls.Add(this.tabPage3);
            this.tabControl1.Controls.Add(this.tabPage2);
            this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControl1.Location = new System.Drawing.Point(0, 0);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(469, 422);
            this.tabControl1.TabIndex = 16;
            //
            // tabPage1
            //
            this.tabPage1.Controls.Add(this.button6);
            this.tabPage1.Controls.Add(this.button4);
            this.tabPage1.Controls.Add(this.playButton);
            this.tabPage1.Controls.Add(this.FRewindButton);
            this.tabPage1.Controls.Add(this.FForwardButton);
            this.tabPage1.Controls.Add(this.button2);
            this.tabPage1.Controls.Add(this.button1);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(461, 396);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Main";
            this.tabPage1.UseVisualStyleBackColor = true;
            //
            // button6
            //
            this.button6.Location = new System.Drawing.Point(72, 130);
            this.button6.Name = "button6";
            this.button6.Size = new System.Drawing.Size(118, 38);
            this.button6.TabIndex = 27;
            this.button6.Text = "List the streams through FFDShow";
            this.button6.UseVisualStyleBackColor = true;
            this.button6.Click += new System.EventHandler(this.getFFDShowStreams_Click);
            //
            // button4
            //
            this.button4.Location = new System.Drawing.Point(262, 128);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(92, 40);
            this.button4.TabIndex = 22;
            this.button4.Text = "List external subtitles";
            this.button4.UseVisualStyleBackColor = true;
            this.button4.Click += new System.EventHandler(this.subtitleFiles_Click);
            //
            // playButton
            //
            this.playButton.Location = new System.Drawing.Point(181, 216);
            this.playButton.Name = "playButton";
            this.playButton.Size = new System.Drawing.Size(93, 35);
            this.playButton.TabIndex = 21;
            this.playButton.Text = "Play/Pause";
            this.playButton.UseVisualStyleBackColor = true;
            this.playButton.Click += new System.EventHandler(this.playButton_Click);
            //
            // FRewindButton
            //
            this.FRewindButton.Location = new System.Drawing.Point(40, 222);
            this.FRewindButton.Name = "FRewindButton";
            this.FRewindButton.Size = new System.Drawing.Size(90, 23);
            this.FRewindButton.TabIndex = 20;
            this.FRewindButton.Text = "Rewind";
            this.FRewindButton.UseVisualStyleBackColor = true;
            this.FRewindButton.Click += new System.EventHandler(this.FRewindButton_Click);
            //
            // FForwardButton
            //
            this.FForwardButton.Location = new System.Drawing.Point(318, 222);
            this.FForwardButton.Name = "FForwardButton";
            this.FForwardButton.Size = new System.Drawing.Size(100, 23);
            this.FForwardButton.TabIndex = 19;
            this.FForwardButton.Text = "Fast Forward";
            this.FForwardButton.UseVisualStyleBackColor = true;
            this.FForwardButton.Click += new System.EventHandler(this.FForwardButton_Click);
            //
            // button2
            //
            this.button2.Location = new System.Drawing.Point(181, 38);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(90, 36);
            this.button2.TabIndex = 17;
            this.button2.Text = "Find FFDShow";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.FFDShow_click);
            //
            // button1
            //
            this.button1.Location = new System.Drawing.Point(106, 342);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(248, 24);
            this.button1.TabIndex = 16;
            this.button1.Text = "Close";
            this.button1.Click += new System.EventHandler(this.close_Click);
            //
            // tabPage3
            //
            this.tabPage3.Controls.Add(this.splitContainer1);
            this.tabPage3.Location = new System.Drawing.Point(4, 22);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage3.Size = new System.Drawing.Size(461, 396);
            this.tabPage3.TabIndex = 2;
            this.tabPage3.Text = "Streams";
            this.tabPage3.UseVisualStyleBackColor = true;
            //
            // splitContainer1
            //
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(3, 3);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            //
            // splitContainer1.Panel1
            //
            this.splitContainer1.Panel1.Controls.Add(this.splitContainer2);
            //
            // splitContainer1.Panel2
            //
            this.splitContainer1.Panel2.Controls.Add(this.button7);
            this.splitContainer1.Size = new System.Drawing.Size(455, 390);
            this.splitContainer1.SplitterDistance = 343;
            this.splitContainer1.TabIndex = 1;
            //
            // splitContainer2
            //
            this.splitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer2.Location = new System.Drawing.Point(0, 0);
            this.splitContainer2.Name = "splitContainer2";
            this.splitContainer2.Orientation = System.Windows.Forms.Orientation.Horizontal;
            //
            // splitContainer2.Panel1
            //
            this.splitContainer2.Panel1.Controls.Add(this.groupBox2);
            //
            // splitContainer2.Panel2
            //
            this.splitContainer2.Panel2.Controls.Add(this.groupBox4);
            this.splitContainer2.Size = new System.Drawing.Size(455, 343);
            this.splitContainer2.SplitterDistance = 176;
            this.splitContainer2.TabIndex = 2;
            //
            // groupBox2
            //
            this.groupBox2.Controls.Add(this.audioStreamlistBox);
            this.groupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox2.Location = new System.Drawing.Point(0, 0);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(455, 176);
            this.groupBox2.TabIndex = 3;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Audio streams";
            //
            // audioStreamlistBox
            //
            this.audioStreamlistBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.audioStreamlistBox.FormattingEnabled = true;
            this.audioStreamlistBox.Location = new System.Drawing.Point(3, 16);
            this.audioStreamlistBox.Name = "audioStreamlistBox";
            this.audioStreamlistBox.Size = new System.Drawing.Size(449, 154);
            this.audioStreamlistBox.TabIndex = 3;
            //
            // groupBox4
            //
            this.groupBox4.Controls.Add(this.subtitleStreamlistBox);
            this.groupBox4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox4.Location = new System.Drawing.Point(0, 0);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(455, 163);
            this.groupBox4.TabIndex = 4;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Subtitle streams";
            //
            // subtitleStreamlistBox
            //
            this.subtitleStreamlistBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.subtitleStreamlistBox.FormattingEnabled = true;
            this.subtitleStreamlistBox.Location = new System.Drawing.Point(3, 16);
            this.subtitleStreamlistBox.Name = "subtitleStreamlistBox";
            this.subtitleStreamlistBox.Size = new System.Drawing.Size(449, 139);
            this.subtitleStreamlistBox.TabIndex = 3;
            //
            // button7
            //
            this.button7.Location = new System.Drawing.Point(158, 3);
            this.button7.Name = "button7";
            this.button7.Size = new System.Drawing.Size(118, 38);
            this.button7.TabIndex = 28;
            this.button7.Text = "List the streams through FFDShow";
            this.button7.UseVisualStyleBackColor = true;
            this.button7.Click += new System.EventHandler(this.getFFDShowStreams_Click);
            //
            // tabPage2
            //
            this.tabPage2.Controls.Add(this.groupBox5);
            this.tabPage2.Controls.Add(this.groupBox1);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(461, 396);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Other tests";
            this.tabPage2.UseVisualStyleBackColor = true;
            //
            // groupBox5
            //
            this.groupBox5.Controls.Add(this.ffdshowParamButton);
            this.groupBox5.Controls.Add(this.label5);
            this.groupBox5.Controls.Add(this.ffdshowParamValueBox);
            this.groupBox5.Controls.Add(this.label4);
            this.groupBox5.Controls.Add(this.ffdshowParamBox);
            this.groupBox5.Location = new System.Drawing.Point(7, 202);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(307, 132);
            this.groupBox5.TabIndex = 1;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "For developers only";
            //
            // ffdshowParamButton
            //
            this.ffdshowParamButton.Location = new System.Drawing.Point(10, 92);
            this.ffdshowParamButton.Name = "ffdshowParamButton";
            this.ffdshowParamButton.Size = new System.Drawing.Size(75, 23);
            this.ffdshowParamButton.TabIndex = 4;
            this.ffdshowParamButton.Text = "Send";
            this.ffdshowParamButton.UseVisualStyleBackColor = true;
            this.ffdshowParamButton.Click += new System.EventHandler(this.ffdshowParamButton_Click);
            //
            // label5
            //
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(7, 56);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(90, 13);
            this.label5.TabIndex = 3;
            this.label5.Text = "Parameter value :";
            //
            // ffdshowParamValueBox
            //
            this.ffdshowParamValueBox.Location = new System.Drawing.Point(103, 53);
            this.ffdshowParamValueBox.Name = "ffdshowParamValueBox";
            this.ffdshowParamValueBox.Size = new System.Drawing.Size(100, 20);
            this.ffdshowParamValueBox.TabIndex = 2;
            //
            // label4
            //
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(6, 26);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(145, 13);
            this.label4.TabIndex = 1;
            this.label4.Text = "FFDShow remote parameter :";
            //
            // ffdshowParamBox
            //
            this.ffdshowParamBox.Location = new System.Drawing.Point(157, 24);
            this.ffdshowParamBox.Name = "ffdshowParamBox";
            this.ffdshowParamBox.Size = new System.Drawing.Size(100, 20);
            this.ffdshowParamBox.TabIndex = 0;
            //
            // groupBox1
            //
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.trackBar2);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.trackBar1);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.textBox1);
            this.groupBox1.Location = new System.Drawing.Point(8, 6);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(306, 189);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "OSD";
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 153);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(81, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Vertical position";
            //
            // trackBar2
            //
            this.trackBar2.Location = new System.Drawing.Point(105, 140);
            this.trackBar2.Maximum = 100;
            this.trackBar2.Name = "trackBar2";
            this.trackBar2.Size = new System.Drawing.Size(184, 45);
            this.trackBar2.TabIndex = 4;
            this.trackBar2.ValueChanged += new System.EventHandler(this.OSDY_ValueChanged);
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 100);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(93, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "Horizontal position";
            //
            // trackBar1
            //
            this.trackBar1.Location = new System.Drawing.Point(105, 87);
            this.trackBar1.Maximum = 100;
            this.trackBar1.Name = "trackBar1";
            this.trackBar1.Size = new System.Drawing.Size(184, 45);
            this.trackBar1.TabIndex = 2;
            this.trackBar1.ValueChanged += new System.EventHandler(this.OSDX_ValueChanged);
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 22);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(28, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Text";
            //
            // textBox1
            //
            this.textBox1.Location = new System.Drawing.Point(47, 19);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBox1.Size = new System.Drawing.Size(242, 62);
            this.textBox1.TabIndex = 0;
            this.textBox1.TextChanged += new System.EventHandler(this.OSD_TextChanged);
            //
            // splitContainer3
            //
            this.splitContainer3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer3.Location = new System.Drawing.Point(3, 3);
            this.splitContainer3.Name = "splitContainer3";
            this.splitContainer3.Orientation = System.Windows.Forms.Orientation.Horizontal;
            //
            // splitContainer3.Panel1
            //
            this.splitContainer3.Panel1.Controls.Add(this.splitContainer4);
            //
            // splitContainer3.Panel2
            //
            this.splitContainer3.Panel2.Controls.Add(this.button8);
            this.splitContainer3.Size = new System.Drawing.Size(455, 390);
            this.splitContainer3.SplitterDistance = 343;
            this.splitContainer3.TabIndex = 1;
            //
            // splitContainer4
            //
            this.splitContainer4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer4.Location = new System.Drawing.Point(0, 0);
            this.splitContainer4.Name = "splitContainer4";
            this.splitContainer4.Orientation = System.Windows.Forms.Orientation.Horizontal;
            //
            // splitContainer4.Panel1
            //
            this.splitContainer4.Panel1.Controls.Add(this.groupBox3);
            this.splitContainer4.Size = new System.Drawing.Size(455, 343);
            this.splitContainer4.SplitterDistance = 176;
            this.splitContainer4.TabIndex = 2;
            //
            // groupBox3
            //
            this.groupBox3.Controls.Add(this.listBox1);
            this.groupBox3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox3.Location = new System.Drawing.Point(0, 0);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(455, 176);
            this.groupBox3.TabIndex = 3;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Audio streams";
            //
            // listBox1
            //
            this.listBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listBox1.FormattingEnabled = true;
            this.listBox1.Location = new System.Drawing.Point(3, 16);
            this.listBox1.Name = "listBox1";
            this.listBox1.Size = new System.Drawing.Size(449, 147);
            this.listBox1.TabIndex = 3;
            //
            // button8
            //
            this.button8.Location = new System.Drawing.Point(158, 3);
            this.button8.Name = "button8";
            this.button8.Size = new System.Drawing.Size(118, 38);
            this.button8.TabIndex = 28;
            this.button8.Text = "List the streams through FFDShow";
            this.button8.UseVisualStyleBackColor = true;
            //
            // FFDShowAPITest
            //
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(469, 422);
            this.Controls.Add(this.tabControl1);
            this.Name = "FFDShowAPITest";
            this.Text = "FFDShow API Test application";
            this.tabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.tabPage3.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.splitContainer2.Panel1.ResumeLayout(false);
            this.splitContainer2.Panel2.ResumeLayout(false);
            this.splitContainer2.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox4.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
            this.splitContainer3.Panel1.ResumeLayout(false);
            this.splitContainer3.Panel2.ResumeLayout(false);
            this.splitContainer3.ResumeLayout(false);
            this.splitContainer4.Panel1.ResumeLayout(false);
            this.splitContainer4.ResumeLayout(false);
            this.groupBox3.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion
    }
}

