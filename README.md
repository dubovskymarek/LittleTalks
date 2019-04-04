
<p>
LittleTalks is very tiny comunication library based on UDP and it's inspirated MQTT protocol.
Instead MQTT it's designed as P2P communication in local network.
This library is portable writed in c language.
Library centralises all UDP implementation in LTPlatformAdapter.c file, that can be overriden to optional platform.
LittleTalks was created for startup canny.tech, where was used on both sides (application and device).
</p>

<b>Building from source</b>
<p>
Just run make to build.
</p>

<b>Documentation</b>
<p>docs/html/index.html</p>

<b>Credits</b>
<p>
LittleTalks was written by Marek Dubovsky dubovskymarek@icloud.com
</p>

<b>Files:</b>

\ref ./lib/LittleTalks.h<br/>
\ref ./lib/LTPlatformAdapter.h<br/>

\section ex1 Example 1
\snippet example1/main.c Example 1

