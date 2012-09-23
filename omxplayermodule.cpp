#include <Python.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <string.h>


#define AV_NOWARN_DEPRECATED

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
};

#include "OMXStreamInfo.h"
#include "utils/log.h"
#include "DllAvUtil.h"
#include "DllAvFormat.h"              
#include "DllAvFilter.h"
#include "DllAvCodec.h"
#include "linux/RBP.h"                
                                
#include "OMXVideo.h"
#include "OMXAudioCodecOMX.h"
#include "utils/PCMRemap.h"
#include "OMXClock.h"
#include "OMXAudio.h"
#include "OMXReader.h"                
#include "OMXPlayerAudio.h"
#include "DllOMX.h"
#include <string>

CRBP g_RBP;
COMXCore g_OMX;                                    
OMXClock *m_av_clock = NULL;
OMXReader m_omx_reader;
COMXStreamInfo m_hints_audio;
OMXPlayerAudio m_player_audio;
OMXPacket *m_omx_pkt = NULL;

std::string deviceString = "omx:local";
bool m_passthrough = false;
bool m_buffer_empty = true;
bool m_stop = false;
double m_incr = 0;
double startpts = 0;
bool m_Pause = false;
bool m_dump_format = false;

void FlushStreams(double pts);

   
static PyObject * omxplayer_Stop(PyObject *self, PyObject *args)
{                                                 
    int sts;

    m_stop = true;
    
    sts = 1;                  
    return PyLong_FromLong(sts);
}

static PyObject * omxplayer_VolumeUp(PyObject *self, PyObject *args)
{                    
    int sts;
    m_player_audio.SetCurrentVolume(m_player_audio.GetCurrentVolume() + 50);

    sts = 1;                 
    return PyLong_FromLong(sts);
}

static PyObject * omxplayer_VolumeDown(PyObject *self, PyObject *args)
{                 
    int sts;
    m_player_audio.SetCurrentVolume(m_player_audio.GetCurrentVolume() - 50);

    sts = 1;                 
    return PyLong_FromLong(sts);
}

static PyObject * omxplayer_Seek(PyObject *self, PyObject *args)
{
    int sts;
    int seek;
    int ok;
    
    ok = PyArg_ParseTuple(args, "i", &seek);

    if(m_omx_reader.CanSeek()) 
      m_incr += seek;

    sts = 1;  
    printf("End of Seek()\n");               
    return PyLong_FromLong(sts);
}

void SetSpeed(int iSpeed)
{
  if(!m_av_clock)
    return;

  if(iSpeed < OMX_PLAYSPEED_PAUSE)
    return;

  m_omx_reader.SetSpeed(iSpeed);

  if(m_av_clock->OMXPlaySpeed() != OMX_PLAYSPEED_PAUSE && iSpeed == OMX_PLAYSPEED_PAUSE)
    m_Pause = true;
  else if(m_av_clock->OMXPlaySpeed() == OMX_PLAYSPEED_PAUSE && iSpeed != OMX_PLAYSPEED_PAUSE)
    m_Pause = false;

  m_av_clock->OMXSpeed(iSpeed);
}
           
static PyObject * omxplayer_Pause(PyObject *self, PyObject *args)
{ 
  int ok;
  int sts;
  ok = PyArg_ParseTuple(args, "i", &m_Pause);

  if(m_Pause)
  {
    SetSpeed(OMX_PLAYSPEED_PAUSE);
    m_av_clock->OMXPause();
  }
  else
  {
    SetSpeed(OMX_PLAYSPEED_NORMAL);
    m_av_clock->OMXResume();
  }
  
  sts = 1;                 
  return PyLong_FromLong(sts);
}
       
static PyObject * omxplayer_GetCurrentPosition(PyObject* self, PyObject* args)
{
	float position = (float)m_player_audio.GetCurrentPTS();
	printf("OmxPlayerModule : position : %f\n", position);
	position /= DVD_TIME_BASE;
	return Py_BuildValue("f",position);
}

static PyObject * omxplayer_GetDuration(PyObject* self, PyObject* args)
{
	int duration  = duration = m_omx_reader.GetStreamLength();
	duration /=1000;
	
	return Py_BuildValue("i",duration);
}

                    
static PyObject * omxplayer_Play(PyObject *self, PyObject *args)
{  
  int sts = 0;  
  struct timespec starttime, endtime; 
  
  g_RBP.Initialize();
  g_OMX.Initialize();
  m_av_clock = new OMXClock(); 
  
  if(!m_omx_reader.Open("./file.mp3", m_dump_format))
  {
    printf("Cannot open OMX Reader\n");
    return PyLong_FromLong(sts); ;
  }
  
  
  
  if(m_dump_format)
  {
    printf("Dump format error\n");
    return PyLong_FromLong(sts);
  }              
  
  if(!m_av_clock->OMXInitialize(false, true))
  {
    printf("OMX Initialize error\n");
    return PyLong_FromLong(sts); 
  }     
  
  m_omx_reader.GetHints(OMXSTREAM_AUDIO, m_hints_audio);  
  
  m_omx_reader.SetActiveStream(OMXSTREAM_AUDIO, true);
  
  if(!m_player_audio.Open(m_hints_audio, m_av_clock, &m_omx_reader, deviceString, 
                                         true, false, true))
  {
    printf("Open error\n");
    return PyLong_FromLong(sts);
  }     
  
  m_omx_reader.GetHints(OMXSTREAM_AUDIO, m_hints_audio);
  m_av_clock->SetSpeed(DVD_PLAYSPEED_NORMAL);
  m_av_clock->OMXStateExecute();
  m_av_clock->OMXStart();
      
  m_stop = false; 
  while(!m_stop)
  {
    if(m_Pause)
    {
      Py_BEGIN_ALLOW_THREADS
      OMXClock::OMXSleep(2);
      Py_END_ALLOW_THREADS
      continue;
    }
  
    if(m_incr != 0)
    {
      int    seek_flags   = 0;
      double seek_pos     = 0;
      double pts          = 0;

      pts = m_av_clock->GetPTS();

      seek_pos = (pts / DVD_TIME_BASE) + m_incr;
      seek_flags = m_incr < 0.0f ? AVSEEK_FLAG_BACKWARD : 0;

      seek_pos *= 1000.0f;

      m_incr = 0;

      if(m_omx_reader.SeekTime(seek_pos, seek_flags, &startpts))
        FlushStreams(startpts);
    }
  
    /* player got in an error state */
    if(m_player_audio.Error())
    {
      printf("audio player error. emergency exit!!!\n");
      return PyLong_FromLong(sts);
    }
  
    if(m_omx_reader.IsEof() && !m_omx_pkt)
    {
      if (!m_player_audio.GetCached())
        break;

      // Abort audio buffering, now we're on our own
      if (m_buffer_empty)
        m_av_clock->OMXResume();

      OMXClock::OMXSleep(10);
      continue;
    }
    
    if(m_player_audio.GetDelay() < 0.1f && !m_buffer_empty)
    {
      if(!m_av_clock->OMXIsPaused())
      {
        m_av_clock->OMXPause();
        m_buffer_empty = true;
        clock_gettime(CLOCK_REALTIME, &starttime);
      }
    }
    
    if(m_player_audio.GetDelay() > (AUDIO_BUFFER_SECONDS * 0.75f) && m_buffer_empty)
    {
      if(m_av_clock->OMXIsPaused())
      {
        m_av_clock->OMXResume();
        m_buffer_empty = false;
      }
    }
    if(m_buffer_empty)
    {
      clock_gettime(CLOCK_REALTIME, &endtime);
      if((endtime.tv_sec - starttime.tv_sec) > 1)
      {
        m_buffer_empty = false;
        m_av_clock->OMXResume();
      }
    }
    
    if(!m_omx_pkt)
      m_omx_pkt = m_omx_reader.Read();

    if(m_omx_pkt && m_omx_pkt->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      if(m_player_audio.AddPacket(m_omx_pkt))
        m_omx_pkt = NULL;
      else
        OMXClock::OMXSleep(10);
    }
    else
    {
      if(m_omx_pkt)
      {
        m_omx_reader.FreePacket(m_omx_pkt);
        m_omx_pkt = NULL;
      }
    }
    
	
	
    Py_BEGIN_ALLOW_THREADS
        OMXClock::OMXSleep(1);
    Py_END_ALLOW_THREADS 
  }
  printf("Main::End of loop\n");   
  m_av_clock->OMXStop();
  m_av_clock->OMXStateIdle();
  m_player_audio.Close();
  
  if(m_omx_pkt)
  {
    m_omx_reader.FreePacket(m_omx_pkt);
    m_omx_pkt = NULL;
  }

  m_omx_reader.Close();
  
  g_OMX.Deinitialize();
  g_RBP.Deinitialize();
    
  sts = 1;
  return PyLong_FromLong(sts);
}

void FlushStreams(double pts)
{
  m_player_audio.Flush();

  if(m_omx_pkt)
  {
    m_omx_reader.FreePacket(m_omx_pkt);
    m_omx_pkt = NULL;
  }

  if(pts != DVD_NOPTS_VALUE)
    m_av_clock->OMXUpdateClock(pts);
}

static PyMethodDef OmxplayerMethods[] = 
{
  {"Play", omxplayer_Play, METH_VARARGS, "Play file"},
  {"Stop", omxplayer_Stop, METH_VARARGS, "Stop playing"},
  {"VolumeUp", omxplayer_VolumeUp, METH_VARARGS, "Volume Up"},
  {"VolumeDown", omxplayer_VolumeDown, METH_VARARGS, "Volume Up"},
  {"Seek", omxplayer_Seek, METH_VARARGS, "Seek"},
  {"Pause", omxplayer_Pause, METH_VARARGS, "Pause"},
  {"GetCurrentPosition", omxplayer_GetCurrentPosition, METH_VARARGS, "Get current position"},
  {"GetDuration", omxplayer_GetDuration, METH_VARARGS, "Get duration"},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef omxplayermodule = 
{
	PyModuleDef_HEAD_INIT,
	"omxplayer", NULL, -1, OmxplayerMethods
};

PyMODINIT_FUNC PyInit_omxplayer(void)
{
	return PyModule_Create(&omxplayermodule);
}

int main(int argc, char *argv[])
{
    /* Add a built-in module, before Py_Initialize */
    PyImport_AppendInittab("omxplayer", PyInit_omxplayer);

    /* Pass argv[0] to the Python interpreter */
    Py_SetProgramName((wchar_t*)argv[0]);

    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();

    /* Optionally import the module; alternatively,
       import can be deferred until the embedded script
       imports it. */
    PyImport_ImportModule("omxplayer");
	
}
