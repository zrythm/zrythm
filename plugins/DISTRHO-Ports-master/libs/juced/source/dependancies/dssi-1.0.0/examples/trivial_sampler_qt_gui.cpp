/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* trivial_sampler_qt_gui.cpp

   DSSI Soft Synth Interface
   Constructed by Chris Cannam, Steve Harris and Sean Bolton

   A straightforward DSSI plugin sampler: Qt GUI.

   This example file is in the public domain.
*/

#include "trivial_sampler_qt_gui.h"
#include "trivial_sampler.h"

#include <qapplication.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qgroupbox.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <math.h>
#include <sndfile.h>

#include "dssi.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/SM/SMlib.h>

static int handle_x11_error(Display *dpy, XErrorEvent *err)
{
    char errstr[256];
    XGetErrorText(dpy, err->error_code, errstr, 256);
    if (err->error_code != BadWindow) {
	std::cerr << "trivial_sampler_qt_gui: X Error: "
		  << errstr << " " << err->error_code
		  << "\nin major opcode:  " << err->request_code << std::endl;
    }
    return 0;
}
#endif

using std::cerr;
using std::endl;

lo_server osc_server = 0;

#define NO_SAMPLE_TEXT "<none loaded>   "

SamplerGUI::SamplerGUI(bool stereo, QString host, QString port,
		       QString controlPath, QString midiPath, QString configurePath,
		       QString exitingPath, QWidget *w) :
    QFrame(w),
    m_controlPath(controlPath),
    m_midiPath(midiPath),
    m_configurePath(configurePath),
    m_exitingPath(exitingPath),
    m_previewWidth(200),
    m_previewHeight(40),
    m_suppressHostUpdate(true),
    m_hostRequestedQuit(false),
    m_ready(false)
{
    m_host = lo_address_new(host, port);

    QGridLayout *layout = new QGridLayout(this, 2, 2, 5, 5);
    
    QGroupBox *sampleBox = new QGroupBox(1, Horizontal, "Sample", this);
    layout->addMultiCellWidget(sampleBox, 0, 0, 0, 1);

    QFrame *sampleFrame = new QFrame(sampleBox);
    QGridLayout *sampleLayout = new QGridLayout(sampleFrame,
						stereo ? 4 : 3, 5, 5, 5);

    sampleLayout->addWidget(new QLabel("File:  ", sampleFrame), 0, 0);

    m_sampleFile = new QLabel(NO_SAMPLE_TEXT, sampleFrame);
    m_sampleFile->setFrameStyle(QFrame::Box | QFrame::Plain);
    sampleLayout->addMultiCellWidget(m_sampleFile, 0, 0, 1, 3);

    m_duration = new QLabel("0.00 sec", sampleFrame);
    sampleLayout->addWidget(m_duration, 2, 1, Qt::AlignLeft);
    m_sampleRate = new QLabel(sampleFrame);
    sampleLayout->addWidget(m_sampleRate, 2, 2, Qt:: AlignCenter);
    m_channels = new QLabel(sampleFrame);
    sampleLayout->addWidget(m_channels, 2, 3, Qt::AlignRight);

    QPixmap pmap(m_previewWidth, m_previewHeight);
    pmap.fill();
    m_preview = new QLabel(sampleFrame);
    m_preview->setFrameStyle(QFrame::Box | QFrame::Plain);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setPaletteBackgroundColor(Qt::white);
    m_preview->setPixmap(pmap);
    sampleLayout->addMultiCellWidget(m_preview, 1, 1, 1, 3);

    QPushButton *loadButton = new QPushButton(" ... ", sampleFrame);
    sampleLayout->addWidget(loadButton, 0, 5);
    connect(loadButton, SIGNAL(pressed()), this, SLOT(fileSelect()));

    QPushButton *testButton = new QPushButton("Test", sampleFrame);
    connect(testButton, SIGNAL(pressed()), this, SLOT(test_press()));
    connect(testButton, SIGNAL(released()), this, SLOT(test_release()));
    sampleLayout->addWidget(testButton, 1, 5);

    if (stereo) {
	m_balanceLabel = new QLabel("Balance:  ", sampleFrame);
	sampleLayout->addWidget(m_balanceLabel, 3, 0);
	m_balance = new QSlider(-100, 100, 25, 0, Qt::Horizontal, sampleFrame);

// X11 also defines a numeric constant Below
#undef Below

	m_balance->setTickmarks(QSlider::Below);
	sampleLayout->addMultiCellWidget(m_balance, 3, 3, 1, 3);
	connect(m_balance, SIGNAL(valueChanged(int)), this, SLOT(balanceChanged(int)));
    } else {
	m_balance = 0;
	m_balanceLabel = 0;
    }

    QGroupBox *tuneBox = new QGroupBox(1, Horizontal, "Tuned playback", this);
    layout->addWidget(tuneBox, 1, 0);
    
    QFrame *tuneFrame = new QFrame(tuneBox);
    QGridLayout *tuneLayout = new QGridLayout(tuneFrame, 2, 2, 5, 5);

    m_retune = new QCheckBox("Enable", tuneFrame);
    m_retune->setChecked(true);
    tuneLayout->addMultiCellWidget(m_retune, 0, 0, 0, 1, Qt::AlignLeft);
    connect(m_retune, SIGNAL(toggled(bool)), this, SLOT(retuneChanged(bool)));

    tuneLayout->addWidget(new QLabel("Base pitch: ", tuneFrame), 1, 0);

    m_basePitch = new QSpinBox(tuneFrame);
    m_basePitch->setMinValue(0);
    m_basePitch->setMaxValue(120);
    m_basePitch->setValue(60);
    tuneLayout->addWidget(m_basePitch, 1, 1);
    connect(m_basePitch, SIGNAL(valueChanged(int)), this, SLOT(basePitchChanged(int)));

    QGroupBox *noteOffBox = new QGroupBox(1, Horizontal, "Note Off", this);
    layout->addWidget(noteOffBox, 1, 1);
    
    QFrame *noteOffFrame = new QFrame(noteOffBox);
    QGridLayout *noteOffLayout = new QGridLayout(noteOffFrame, 2, 2, 5, 5);

    m_sustain = new QCheckBox("Enable", noteOffFrame);
    m_sustain->setChecked(true);
    noteOffLayout->addMultiCellWidget(m_sustain, 0, 0, 0, 1, Qt::AlignLeft);
    connect(m_sustain, SIGNAL(toggled(bool)), this, SLOT(sustainChanged(bool)));
    
    noteOffLayout->addWidget(new QLabel("Release: ", noteOffFrame), 1, 0);
    
    m_release = new QSpinBox(noteOffFrame);
    m_release->setMinValue(0);
    m_release->setMaxValue(int(Sampler_RELEASE_MAX * 1000));
    m_release->setValue(0);
    m_release->setSuffix("ms");
    m_release->setLineStep(10);
    noteOffLayout->addWidget(m_release, 1, 1);
    connect(m_release, SIGNAL(valueChanged(int)), this, SLOT(releaseChanged(int)));

    // cause some initial updates
    retuneChanged     (m_retune    ->isChecked());
    basePitchChanged  (m_basePitch ->value());
    sustainChanged    (m_sustain   ->isChecked());
    releaseChanged    (m_release   ->value());
    if (stereo) {
	balanceChanged(m_balance   ->value());
    }

    QTimer *myTimer = new QTimer(this);
    connect(myTimer, SIGNAL(timeout()), this, SLOT(oscRecv()));
    myTimer->start(0, false);

    m_suppressHostUpdate = false;
}

void
SamplerGUI::generatePreview(QString path)
{
    SF_INFO info;
    SNDFILE *file;
    QPixmap pmap(m_previewWidth, m_previewHeight);
    pmap.fill();
    
    info.format = 0;
    file = sf_open(path.latin1(), SFM_READ, &info);

    if (file && info.frames > 0) {

	float binSize = (float)info.frames / m_previewWidth;
	float peak[2] = { 0.0f, 0.0f }, mean[2] = { 0.0f, 0.0f };
	float *frame = (float *)malloc(info.channels * sizeof(float));
	int bin = 0;

	QPainter paint(&pmap);

	for (size_t i = 0; i < info.frames; ++i) {

	    sf_readf_float(file, frame, 1);

	    if (fabs(frame[0]) > peak[0]) peak[0] = fabs(frame[0]);
	    mean[0] += fabs(frame[0]);
		
	    if (info.channels > 1) {
		if (fabs(frame[1]) > peak[1]) peak[1] = fabs(frame[1]);
		mean[1] += fabs(frame[1]);
	    }

	    if (i == size_t((bin + 1) * binSize)) {

		float silent = 1.0 / float(m_previewHeight);

		if (info.channels == 1) {
		    mean[1] = mean[0];
		    peak[1] = peak[0];
		}

		mean[0] /= binSize;
		mean[1] /= binSize;

		int m = m_previewHeight / 2;

		paint.setPen(Qt::black);
		paint.drawLine(bin, m, bin, int(m - m * peak[0]));
		if (peak[0] > silent && peak[1] > silent) {
		    paint.drawLine(bin, m, bin, int(m + m * peak[1]));
		}

		paint.setPen(Qt::gray);
		paint.drawLine(bin, m, bin, int(m - m * mean[0]));
		if (mean[0] > silent && mean[1] > silent) {
		    paint.drawLine(bin, m, bin, int(m + m * mean[1]));
		}

		paint.setPen(Qt::black);
		paint.drawPoint(bin, int(m - m * peak[0]));
		if (peak[0] > silent && peak[1] > silent) {
		    paint.drawPoint(bin, int(m + m * peak[1]));
		}

		mean[0] = mean[1] = 0.0f;
		peak[0] = peak[1] = 0.0f;

		++bin;
	    }
	}

	int duration = int(100.0 * float(info.frames) / float(info.samplerate));
	std::cout << "duration " << duration << std::endl;
	m_duration->setText(QString("%1.%2%3 sec")
			    .arg(duration / 100)
			    .arg((duration / 10) % 10)
			    .arg((duration % 10)));
	m_sampleRate->setText(QString("%1 Hz")
			      .arg(info.samplerate));
	m_channels->setText(info.channels > 1 ? (m_balance ? "stereo" : "stereo (to mix)") : "mono");
	if (m_balanceLabel) {
	    m_balanceLabel->setText(info.channels == 1 ? "Pan:  " : "Balance:  ");
	}

    } else {
	m_duration->setText("0.00 sec");
	m_sampleRate->setText("");
	m_channels->setText("");
    }

    if (file) sf_close(file);

    m_preview->setPixmap(pmap);

}

void
SamplerGUI::setProjectDirectory(QString dir)
{
    QFileInfo info(dir);
    if (info.exists() && info.isDir() && info.isReadable()) {
	m_projectDir = dir;
    }
}

void
SamplerGUI::setSampleFile(QString file)
{
    m_suppressHostUpdate = true;
    m_sampleFile->setText(QFileInfo(file).fileName());
    m_file = file;
    generatePreview(file);
    m_suppressHostUpdate = false;
}

void
SamplerGUI::setRetune(bool retune)
{
    m_suppressHostUpdate = true;
    m_retune->setChecked(retune);
    m_basePitch->setEnabled(retune);
    m_suppressHostUpdate = false;
}

void
SamplerGUI::setBasePitch(int pitch)
{
    m_suppressHostUpdate = true;
    m_basePitch->setValue(pitch);
    m_suppressHostUpdate = false;
}

void
SamplerGUI::setSustain(bool sustain)
{
    m_suppressHostUpdate = true;
    m_sustain->setChecked(sustain);
    m_release->setEnabled(sustain);
    m_suppressHostUpdate = false;
}

void
SamplerGUI::setRelease(int ms)
{
    m_suppressHostUpdate = true;
    m_release->setValue(ms);
    m_suppressHostUpdate = false;
}

void
SamplerGUI::setBalance(int balance)
{
    m_suppressHostUpdate = true;
    if (m_balance) {
	m_balance->setValue(balance);
    }
    m_suppressHostUpdate = false;
}

void
SamplerGUI::retuneChanged(bool retune)
{
    if (!m_suppressHostUpdate) {
	lo_send(m_host, m_controlPath, "if", Sampler_RETUNE, retune ? 1.0 : 0.0);
    }
    m_basePitch->setEnabled(retune);
}

void
SamplerGUI::basePitchChanged(int value)
{
    if (!m_suppressHostUpdate) {
	lo_send(m_host, m_controlPath, "if", Sampler_BASE_PITCH, (float)value);
    }
}

void
SamplerGUI::sustainChanged(bool on)
{
    if (!m_suppressHostUpdate) {
	lo_send(m_host, m_controlPath, "if", Sampler_SUSTAIN, on ? 0.0 : 1.0);
    }
    m_release->setEnabled(on);
}

void
SamplerGUI::releaseChanged(int release)
{
    if (!m_suppressHostUpdate) {
	float v = (float)release / 1000.0;
	if (v < Sampler_RELEASE_MIN) v = Sampler_RELEASE_MIN;
	lo_send(m_host, m_controlPath, "if", Sampler_RELEASE, v);
    }
}

void
SamplerGUI::balanceChanged(int balance)
{
    if (!m_suppressHostUpdate) {
	float v = (float)balance / 100.0;
	lo_send(m_host, m_controlPath, "if", Sampler_BALANCE, v);
    }
}

void
SamplerGUI::fileSelect()
{
    QString orig = m_file;
    if (!orig) {
	if (m_projectDir) {
	    orig = m_projectDir;
	} else {
	    orig = ".";
	}
    }

    QString path = QFileDialog::getOpenFileName
	(orig, "Audio files (*.wav *.aiff)", this, "file select",
	 "Select an audio sample file");

    if (path) {

	SF_INFO info;
	SNDFILE *file;

	info.format = 0;
	file = sf_open(path, SFM_READ, &info);

	if (!file) {
	    QMessageBox::warning
		(this, "Couldn't load audio file",
		 QString("Couldn't load audio sample file '%1'").arg(path),
		 QMessageBox::Ok, 0);
	    return;
	}
	
	if (info.frames > Sampler_FRAMES_MAX) {
	    QMessageBox::warning
		(this, "Couldn't use audio file",
		 QString("Audio sample file '%1' is too large (%2 frames, maximum is %3)").arg(path).arg((int)info.frames).arg(Sampler_FRAMES_MAX),
		 QMessageBox::Ok, 0);
	    sf_close(file);
	    return;
	} else {
	    sf_close(file);
	    lo_send(m_host, m_configurePath, "ss", "load", path.latin1());
	    setSampleFile(path);
	}
    }
}

void
SamplerGUI::test_press()
{
    unsigned char noteon[4] = { 0x00, 0x90, 0x3C, 60 };

    lo_send(m_host, m_midiPath, "m", noteon);
}

void
SamplerGUI::oscRecv()
{
    if (osc_server) {
	lo_server_recv_noblock(osc_server, 1);
    }
}

void
SamplerGUI::test_release()
{
    unsigned char noteoff[4] = { 0x00, 0x90, 0x3C, 0x00 };

    lo_send(m_host, m_midiPath, "m", noteoff);
}

void
SamplerGUI::aboutToQuit()
{
    if (!m_hostRequestedQuit) lo_send(m_host, m_exitingPath, "");
}

SamplerGUI::~SamplerGUI()
{
    lo_address_free(m_host);
}


void
osc_error(int num, const char *msg, const char *path)
{
    cerr << "Error: liblo server error " << num
	 << " in path \"" << (path ? path : "(null)")
	 << "\": " << msg << endl;
}

int
debug_handler(const char *path, const char *types, lo_arg **argv,
	      int argc, void *data, void *user_data)
{
    int i;

    cerr << "Warning: unhandled OSC message in GUI:" << endl;

    for (i = 0; i < argc; ++i) {
	cerr << "arg " << i << ": type '" << types[i] << "': ";
        lo_arg_pp((lo_type)types[i], argv[i]);
	cerr << endl;
    }

    cerr << "(path is <" << path << ">)" << endl;
    return 1;
}

int
configure_handler(const char *path, const char *types, lo_arg **argv,
		  int argc, void *data, void *user_data)
{
    SamplerGUI *gui = static_cast<SamplerGUI *>(user_data);
    const char *key = (const char *)&argv[0]->s;
    const char *value = (const char *)&argv[1]->s;

    if (!strcmp(key, "load")) {
	gui->setSampleFile(value);
    } else if (!strcmp(key, DSSI_PROJECT_DIRECTORY_KEY)) {
	gui->setProjectDirectory(value);
    }

    return 0;
}

int
rate_handler(const char *path, const char *types, lo_arg **argv,
	     int argc, void *data, void *user_data)
{
    return 0;
}

int
show_handler(const char *path, const char *types, lo_arg **argv,
	     int argc, void *data, void *user_data)
{
    SamplerGUI *gui = static_cast<SamplerGUI *>(user_data);
    while (!gui->ready()) sleep(1);
    if (gui->isVisible()) gui->raise();
    else {
	QRect geometry = gui->geometry();
	QPoint p(QApplication::desktop()->width()/2 - geometry.width()/2,
		 QApplication::desktop()->height()/2 - geometry.height()/2);
	gui->move(p);
	gui->show();
    }

    return 0;
}

int
hide_handler(const char *path, const char *types, lo_arg **argv,
	     int argc, void *data, void *user_data)
{
    SamplerGUI *gui = static_cast<SamplerGUI *>(user_data);
    gui->hide();
    return 0;
}

int
quit_handler(const char *path, const char *types, lo_arg **argv,
	     int argc, void *data, void *user_data)
{
    SamplerGUI *gui = static_cast<SamplerGUI *>(user_data);
    gui->setHostRequestedQuit(true);
    qApp->quit();
    return 0;
}

int
control_handler(const char *path, const char *types, lo_arg **argv,
		int argc, void *data, void *user_data)
{
    SamplerGUI *gui = static_cast<SamplerGUI *>(user_data);

    if (argc < 2) {
	cerr << "Error: too few arguments to control_handler" << endl;
	return 1;
    }

    const int port = argv[0]->i;
    const float value = argv[1]->f;

    switch (port) {

    case Sampler_RETUNE:
	gui->setRetune(value < 0.001f ? false : true);
	break;

    case Sampler_BASE_PITCH:
	gui->setBasePitch((int)value);
	break;

    case Sampler_SUSTAIN:
	gui->setSustain(value < 0.001f ? true : false);
	break;

    case Sampler_RELEASE:
	gui->setRelease(value < (Sampler_RELEASE_MIN + 0.000001f) ?
			0 : (int)(value * 1000.0 + 0.5));
	break;

    case Sampler_BALANCE:
	gui->setBalance((int)(value * 100.0));
	break;

    default:
	cerr << "Warning: received request to set nonexistent port " << port << endl;
    }

    return 0;
}

int
main(int argc, char **argv)
{
    cerr << "trivial_sampler_qt_gui starting..." << endl;

    QApplication application(argc, argv);

    if (application.argc() != 5) {
	cerr << "usage: "
	     << application.argv()[0] 
	     << " <osc url>"
	     << " <plugin dllname>"
	     << " <plugin label>"
	     << " <user-friendly id>"
	     << endl;
	return 2;
    }

#ifdef Q_WS_X11
    XSetErrorHandler(handle_x11_error);
#endif

    char *url = application.argv()[1];

    char *host = lo_url_get_hostname(url);
    char *port = lo_url_get_port(url);
    char *path = lo_url_get_path(url);

    char *label = application.argv()[3];
    bool stereo = false;
    if (QString(label).lower() == QString(Sampler_Stereo_LABEL).lower()) {
	stereo = true;
    }

    SamplerGUI gui(stereo, host, port,
		   QString("%1/control").arg(path),
		   QString("%1/midi").arg(path),
		   QString("%1/configure").arg(path),
		   QString("%1/exiting").arg(path),
		   0);
		 
    application.setMainWidget(&gui);
    gui.setCaption(application.argv()[4]);

    QString myControlPath = QString("%1/control").arg(path);
    QString myConfigurePath = QString("%1/configure").arg(path);
    QString myRatePath = QString("%1/sample-rate").arg(path);
    QString myShowPath = QString("%1/show").arg(path);
    QString myHidePath = QString("%1/hide").arg(path);
    QString myQuitPath = QString("%1/quit").arg(path);

    osc_server = lo_server_new(NULL, osc_error);
    lo_server_add_method(osc_server, myControlPath, "if", control_handler, &gui);
    lo_server_add_method(osc_server, myConfigurePath, "ss", configure_handler, &gui);
    lo_server_add_method(osc_server, myRatePath, "i", rate_handler, &gui);
    lo_server_add_method(osc_server, myShowPath, "", show_handler, &gui);
    lo_server_add_method(osc_server, myHidePath, "", hide_handler, &gui);
    lo_server_add_method(osc_server, myQuitPath, "", quit_handler, &gui);
    lo_server_add_method(osc_server, NULL, NULL, debug_handler, &gui);

    lo_address hostaddr = lo_address_new(host, port);
    lo_send(hostaddr,
	    QString("%1/update").arg(path),
	    "s",
	    QString("%1%2").arg(lo_server_get_url(osc_server)).arg(path+1).data());

    QObject::connect(&application, SIGNAL(aboutToQuit()), &gui, SLOT(aboutToQuit()));

    gui.setReady(true);
    return application.exec();
}

