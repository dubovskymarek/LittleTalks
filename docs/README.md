<b>LittleTalks</b>
<p>
LittleTalks is very tiny comunication library based on UDP and it's inspirated MQTT protocol.
Instead MQTT it's designed as P2P communication in local network.
This library is portable writed in c language.
Library centralises all UDP implementation in LTPlatformAdapter.c file, that can be overriden to optional platform.
LittleTalks was created for startup canny.tech, where was used on both sides (application and device).
</p>

<b>Building from source</b><br/>
Just run make to build.<br/>

<b>Documentation</b><br/>
docs/html/index.html<br/>

<b>Credits</b><br/>
LittleTalks was written by Marek Dubovsky dubovskymarek@icloud.com<br/>

<b>Files:</b>

\ref ./lib/LittleTalks.h<br/>
\ref ./lib/LTPlatformAdapter.h<br/>

<b>Quick start</b>
<p>Example1 demonstrates basic sending and receiving 2 topics between 2 or more users.</p>

\section ex1 Example 1
\snippet example1/main.c Example 1

