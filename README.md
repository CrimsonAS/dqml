A simple live coding environment for QML



Standalone use:

 > dqml file.qml

Will track the directory where file.qml is located and continuously update
the view depending on changes in the filesystem when qmlfiles and images there
are changed.

To add more directories to be tracked, specify them with --track [path]:

 > dqml --track qml --track images --track . file.qml



Remote use:

On the host machine, run:

 > dqml --monitor address port 

Will track changes in the current directory and push them to the server. 
If the server is disconnected or not yet ready, it will keep trying to 
reconnect to the specified address.

Then on the server, run: 

 > dqml --server port file.qml

To add more directories to track, specify them with --track [id] [path]. 
The [id] is used to identify a directory between monitor and server. Say
that your application is located in /home/me/myapp on the host machine
and is deployed to /usr/shared/myapp on the target machine and you are
interested in exposing the subdirectories 'qml' and 'images' the commands
would be as follows:

On the host machine:

 > dqml --monitor address port --track qmlfiles /home/me/myapp/qml --trace /home/me/myapp/images

On the target machine: 

 > dqml --server port --track qmlfiles /usr/share/myapp/qml --track /usr/share/myapp/images

