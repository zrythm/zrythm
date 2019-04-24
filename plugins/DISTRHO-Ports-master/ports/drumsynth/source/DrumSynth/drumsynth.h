//Make a .WAV file from a .DS file

/*
int ds2wav (char *dsfile, char *wavfile);
int ds2mem (char *dsfile, short *buffer, float t, float o, float n, float b, float tune, float time);
int ds2buf (long samples, short *buffer);
*/

/*  Visual Basic Declarations:

Declare Function ds2wav Lib "ds2wav.dll" (ByVal dsfile As String, ByVal wavfile As String) As Long
'
' Makes a WAV file from a DS file
' Returns: 0 - success
'          1 - version failed (this DLL out of date)
'          2 - input failed
'          3 - output failed

Declare Function ds2mem Lib "ds2wav.dll" (ByVal dsfile As String, ByVal buffer As Integer, _
                        ByVal t As Single, ByVal o As Single, ByVal n As Single, _
                        ByVal b As Single, ByVal tune As Single, ByVal time As Single) As Long
'
' Makes a waveform in memory from a DS file (ds2wav sets buffer)
' Returns number of 16-bit samples available in ds2wav's buffer[] or zero if failed
'
' t    = tone      (volume for each section: suggest square-law range 0.0...1.0...4.0)
' o    = overtone
' n    = noise
' b    = noise bands
' tune = tuning offset in semitones
' time = time scaling factor


Declare Function ds2buf Lib "ds2wav.dll" (ByVal samples As Long, buffer As Integer) As Long
'
' Copies a number of waveform samples to your own buffer
' Returns number of 16-bit samples copied into buffer[] or zero if failed
'
*/

