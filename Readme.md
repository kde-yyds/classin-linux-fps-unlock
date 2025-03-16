# Classin for Linux FPS Unlock
Classin for linux limits fps to a very low value in a classroom, so the UI is very laggy. This simple hook lib can make it smooth.
# How it works
At first, I found that using faketime to accelerate it can make it smoother.
So we hook some functions in QT to partly accelerate the rendering process, and at the same time does not accelerate other parts.
# Usage
```
git clone https://github.com/kde-yyds/classin-linux-fps-unlock
cd classin-linux-fps-unlock
gcc -shared -fPIC hook.c -o libClassinFpsUnlock.so -ldl
sudo cp -r libClassFpsUnlock.so /opt/apps/classin/lib/libClassinFpsUnlock.so
export LD_PRELOAD=/opt/apps/classin/lib/libClassinFpsUnlock.so
/opt/apps/classin/ClassIn
```
# Supported Environmental Varibles
`QT_RATE_FACTOR`: Accelerate to custom accelerate rate. If not set by default it's 10x.  
`QT_FRAME_UNLOCK_DEBUG`: Print some debug information.
