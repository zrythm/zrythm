/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* trivial_sampler_qt_gui.h

   DSSI Soft Synth Interface
   Constructed by Chris Cannam, Steve Harris and Sean Bolton

   A straightforward DSSI plugin sampler Qt GUI.

   This example file is in the public domain.
*/

#ifndef _TRIVIAL_SAMPLER_QT_GUI_H_INCLUDED_
#define _TRIVIAL_SAMPLER_QT_GUI_H_INCLUDED_

#include <qframe.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qslider.h>
#include <qlayout.h>

extern "C" {
#include <lo/lo.h>
}

class SamplerGUI : public QFrame
{
    Q_OBJECT

public:
    SamplerGUI(bool stereo, QString host, QString port,
	       QString controlPath, QString midiPath, QString programPath,
	       QString exitingPath, QWidget *w = 0);
    virtual ~SamplerGUI();

    bool ready() const { return m_ready; }
    void setReady(bool ready) { m_ready = ready; }

    void setHostRequestedQuit(bool r) { m_hostRequestedQuit = r; }

public slots:
    void setSampleFile(QString file);
    void setRetune(bool retune);
    void setBasePitch(int pitch);
    void setSustain(bool sustain);
    void setRelease(int ms);
    void setBalance(int balance);
    void setProjectDirectory(QString dir);
    void aboutToQuit();

protected slots:
    void fileSelect();
    void retuneChanged(bool);
    void basePitchChanged(int);
    void sustainChanged(bool);
    void releaseChanged(int);
    void balanceChanged(int);
    void test_press();
    void test_release();
    void oscRecv();
    void generatePreview(QString file);

protected:
    QLabel *m_sampleFile;
    QLabel *m_duration;
    QLabel *m_channels;
    QLabel *m_sampleRate;
    QLabel *m_preview;
    QLabel *m_balanceLabel;
    QCheckBox *m_retune;
    QSpinBox *m_basePitch;
    QCheckBox *m_sustain;
    QSpinBox *m_release;
    QSlider *m_balance;

    lo_address m_host;
    QString m_controlPath;
    QString m_midiPath;
    QString m_configurePath;
    QString m_exitingPath;

    QString m_file;
    QString m_projectDir;
    int m_previewWidth;
    int m_previewHeight;

    bool m_suppressHostUpdate;
    bool m_hostRequestedQuit;
    bool m_ready;
};


#endif
