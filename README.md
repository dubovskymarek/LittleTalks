
\brief LittleTalks is very tiny comunication library based on UDP and it's inspirated MQTT protocol.
\brief Instead MQTT it's designed as P2P communication in local network.

\brief This library is portable writed in c language.
\brief Library centralises all UDP implementation in LTPlatformAdapter.c file, that can be overriden to optional platform.
\brief LittleTalks was created for startup canny.tech, where was used on both sides (application and device).


<b>Building from source</b>
On Windows and Mac, use cmake to build. On other platforms, just run make to build.

<b>Credits</b>
LittleTalks was written by Marek Dubovsky dubovskymarek@icloud.com

<b>Files:</b>

\ref ./lib/LittleTalks.h<br/>
\ref ./lib/LTPlatformAdapter.h<br/>

\section ex1 Example 1
\snippet example1/main.c Example 1

