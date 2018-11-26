#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/human/Projects/gr-pipe/python
export GR_CONF_CONTROLPORT_ON=False
export PATH=/home/human/Projects/gr-pipe/test_build_newcmake/python:$PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH
export PYTHONPATH=/home/human/Projects/gr-pipe/test_build_newcmake/swig:$PYTHONPATH
/usr/bin/python /home/human/Projects/gr-pipe/python/qa_pipe.py 
