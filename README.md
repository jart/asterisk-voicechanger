
              .--.                              .--.
            .'(`                               /  ..\
         __.>\ '.  _.---,._,'             ____.'  _o/
       /.--.  : |/' _.--.<                '--.     |.__
    _..-'    `\     /'     `'             _.-'     /--'
     >_.-``-. `Y  /' _.---._____     _.--'        /
    '` .-''. \|:  \.'   ___, .-'`   ~'--....___.-'
     .'--._ `-:  \/   /'    \\
         /.'`\ :;    /'       `-.
        -`    |     |
              :.; : |             Asterisk Voice Changer
              |:    |                  Version o.9
              |     |
              :. :  |      Copyright (c) 2005-2012 Justine Tunney
            .jgs    ;             Keep it open source pigs
            /:::.    `\
----------------------------------------------------------------------

Website: <http://lobstertech.com/code/voicechanger/>
Email: <jtunney@lobstertech.com>

This is a module for Asterisk that can be used to alter the pitch of a
person's voice in real time during a telephone conversation.


Installation
============

Requirements for this release:

 - Asterisk 10+
 - SoundTouch >= 1.3.1   (apt-get install libsoundtouch-dev)
 - make sure tha Asterisk Modules exist at /usr/include/asterisk ; if not run the following command :
 
```
    cp -R /Path/to/asterisk-20/include/asterisk /usr/include 
```

If you're running Asterisk 1.6, run: git checkout 8428491e2b

Asterisk 1.8 isn't supported at this time.

Once you've installed the software listed above, run these commands::
```
  make
  make install
  asterisk -rx 'module load app_voicechanger.so'
  asterisk -rx 'core show application VoiceChanger'
```

Examples
========

Add something like the following to your dialplan::

  #### scary monster voice
```
  exten => 9000,1,VoiceChanger(-5.0)
  exten => 9000,2,Dial(sip/lolcat@example.com)
  exten => 9000,3,StopVoiceChanger() ; not required
```
  #### chipmunk voice
```  
  exten => 9001,1,VoiceChanger(5.0)
  exten => 9001,2,Dial(sip/lolcat@example.com)
  exten => 9001,3,StopVoiceChanger() ; not required
```
