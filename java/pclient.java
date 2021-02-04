import java.applet.*;
import java.awt.*;
import java.awt.image.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;
import java.util.*;
import java.util.zip.*;
import java.lang.*;


public class pclient extends Applet {

    // Utilities

    public static String[] splitString(String s, char delim) {
        // StringTokenizer drops empty tokens :-(
        int count = 1;
        int length = s.length();
        for (int i=0; i<length; i++) {
            if (s.charAt(i) == delim)
                count++;
        }
        String[] result = new String[count];
        int origin = 0;
        count = 0;
        for (int i=0; i<length; i++) {
            if (s.charAt(i) == delim) {
                result[count++] = s.substring(origin, i);
                origin = i+1;
            }
        }
        result[count] = s.substring(origin);
        return result;
    }

    public static String readLine(InputStream st) throws IOException {
        String result = "";
        int c;
        while ((c = st.read()) != (byte) '\n') {
            if (c < 0)
                throw new IOException("unexpected end of stream");
            result += (char) c;
        }
        return result.trim();
    }

    public static void readAll(InputStream st, byte[] buffer, int off, int len)
                                           throws IOException {
        while (len > 0) {
            int count = st.read(buffer, off, len);
            if (count <= 0)
                throw new IOException("unexpected end of data");
            off += count;
            len -= count;
        }
    }

    public static Color makeColor(int color) {
        return new Color(color & 0xFF,
                         (color >> 8) & 0xFF,
                         (color >> 16) & 0xFF);
    }

    public static InputStream decompresser(byte[] data, int off, int len) {
        return new InflaterInputStream(new ByteArrayInputStream(data, off, len));
    }

    /*class ArraySlice {
        public byte[] array;
        public int ofs, size;
        ArraySlice(byte[] aarray, int aofs, int asize) {
            array = aarray;
            ofs = aofs;
            size = asize;
        }
        }*/

    public void debug(Throwable e) {
        showStatus(e.toString());
        e.printStackTrace();
    }

    // Bitmaps and icons

    class Bitmap {

        int w, h;
        public int[] pixelData;

        Bitmap(pclient client, InputStream st, int keycol) throws IOException {
            String line = readLine(st);
            if (!"P6".equals(line)) throw new IOException("not a P6 PPM image");
            while ((line = readLine(st)).startsWith("#"))
                ;
            String[] wh = splitString(line, ' ');
            if (wh.length != 2) throw new IOException("invalid PPM image size");
            w = Integer.parseInt(wh[0]);
            h = Integer.parseInt(wh[1]);
            line = readLine(st);
            if (!"255".equals(line))
                throw new IOException("not a 255-levels PPM image");

            // over-allocate an extra uninitialized line at the bottom of the
            // image to work around a bug in the MemoryImageSource constructor
            pixelData = new int[w*(h+1)];
            int target = 0;
            int w3 = 3*w;
            byte[] lineBuffer = new byte[w3];
            for (int y=0; y<h; y++) {
                readAll(st, lineBuffer, 0, w3);
                for (int x3=0; x3<w3; x3+=3) {
                    int rgb = (((((int) lineBuffer[x3]  ) & 0xFF) << 16) |
                               ((((int) lineBuffer[x3+1]) & 0xFF) << 8 ) |
                               ((((int) lineBuffer[x3+2]) & 0xFF)      ));
                    if (rgb == keycol)
                        rgb = 0;
                    else
                        rgb |= 0xFF << 24;
                    pixelData[target++] = rgb;
                }
            }
        }

        public Image extractIcon(int x1, int y1, int w1, int h1) {
            return createImage(new MemoryImageSource(w1, h1, pixelData,
                                                     x1 + y1*w, w));
        }
    }
    
    // Host choosing

    public static final int defaultPort = 8056;
    public static final String pingMessage = "pclient-game-ping";
    public static final String pongMessage = "server-game-pong";

    public Socket pickHost(String udphostname, int port)
                                             throws IOException {
        InetAddress addr = InetAddress.getByName(udphostname);
        byte[] msg = pingMessage.getBytes("UTF8");
        DatagramPacket outp = new DatagramPacket(msg, msg.length, addr, port);
        byte[] buffer = new byte[200];
        DatagramPacket inp = new DatagramPacket(buffer, buffer.length);
        DatagramSocket s = new DatagramSocket();
        showStatus("Looking for a game server on "+udphostname+":"+
                   Integer.toString(port)+"...");
        s.send(outp);
        
        s.receive(inp);
        String inpmsg = new String(inp.getData(), 0, inp.getLength(), "UTF8");
        String[] data = splitString(inpmsg, ':');
        //System.out.println(inpmsg);
        //System.out.println(data.length);
        //System.out.println("<<<"+data[0]+">>>");
        if (data.length >= 4 && pongMessage.equals(data[0])) {
            InetAddress result;
            if (data[2].length() == 0) {
                result = inp.getAddress();
            }
            else {
                result = InetAddress.getByName(data[2]);
            }
            port = Integer.parseInt(data[3]);
            showStatus("Connecting to "+data[1]+" at "+
                       result.toString()+":"+Integer.toString(port)+"...");
            return new Socket(result, port);
        }
        else
            throw new IOException("got an unexpected answer from " +
                                  inp.getAddress().toString());
    }

    // Game state

    class Player {
        public int     pid;
        public boolean playing;
        public boolean local;
        public Image   icon;
        public int     xmin, xmax;
    }

    class KeyName {
        public String  keyname;
        public int     keyid;
        public Image[] keyicons;
        public KeyName next;
        public int     newkeycode;
    }

    public Player[] players = new Player[0];
    public KeyName keys = null;
    public Hashtable keycodes = new Hashtable();
    public boolean taskbarfree = false;
    public boolean taskbarmode = false;
    public KeyName keydefinition_k = null;
    public int keydefinition_pid;
    public Image[] iconImages = new Image[0];
    public Bitmap[] bitmaps = new Bitmap[0];

    public Player getPlayer(int id) {
        if (id >= players.length) {
            Player[] newply = new Player[id+1];
            System.arraycopy(players, 0, newply, 0, players.length);
            players = newply;
        }
        if (players[id] == null) {
            players[id] = new Player();
            players[id].pid = id;
            players[id].playing = false;
            players[id].local = false;
        }
        return players[id];
    }

    public Player nextPlayer(Player prev) {
        int i;
        for (i=prev.pid+1; i<players.length; i++)
            if (players[i] != null)
                return players[i];
        return null;
    }

    public Player firstPlayer() {
        int i;
        for (i=0; i<players.length; i++)
            if (players[i] != null)
                return players[i];
        return null;
    }

    public void setTaskbar(boolean nmode) {
        if (taskbarfree) {
            Player p;
            boolean nolocalplayer = true;
            for (p=firstPlayer(); p!=null; p=nextPlayer(p))
                if (p.local)
                    nolocalplayer = false;
            boolean prevmode = taskbarmode;
            taskbarmode = nmode || nolocalplayer || keydefinition_k != null;
            if (prevmode != taskbarmode)
                repaint();
        }
    }

    public final Image getIcon(int ico) {
        if (ico < 0 || ico >= iconImages.length)
            return null;
        else
            return iconImages[ico];
    }

    public void setIcon(int ico, Image img) {
        if (ico >= iconImages.length) {
            Image[] newico = new Image[ico+1];
            System.arraycopy(iconImages, 0, newico, 0, iconImages.length);
            iconImages = newico;
        }
        iconImages[ico] = img;
    }

    public void setBitmap(int n, Bitmap bmp) {
        if (n >= bitmaps.length) {
            Bitmap[] newbmp = new Bitmap[n+1];
            System.arraycopy(bitmaps, 0, newbmp, 0, bitmaps.length);
            bitmaps = newbmp;
        }
        bitmaps[n] = bmp;
    }

    // Sprites

    class Sprite {
        public int x, y, ico;
        public Image bkgnd;

        public final boolean draw(pclient client, Image backBuffer,
                                  Graphics backGC) {
            Image iconImage = client.getIcon(ico);
            if (iconImage == null) {
                ico = -1;
                return false;
            }
            int w = iconImage.getWidth(client);
            int h = iconImage.getHeight(client);

            if (bkgnd == null || bkgnd.getWidth(client) != w ||
                                 bkgnd.getHeight(client) != h) {
                bkgnd = client.createImage(w, h);
            }
            bkgnd.getGraphics().drawImage(backBuffer, -x, -y, client);
            backGC.drawImage(iconImage, x, y, client);
            //System.out.println("Draw  at "+Integer.toString(x)+", "+
            //                   Integer.toString(y));
            return true;
        }
        
        public final void erase(pclient client, Graphics backGC) {
            if (ico != -1) {
                //System.out.println("Erase at "+Integer.toString(x)+", "+
                //                   Integer.toString(y));
                backGC.drawImage(bkgnd, x, y, client);
            }
        }
    }

    // Playfield

    class Playfield {
        public static final int TASKBAR_HEIGHT = 48;

        public pclient client;
        public int pfwidth, pfheight;
        public Image backBuffer;
        public Sprite[] sprites = new Sprite[0];
        public int numSprites = 0;
        public byte[] pendingBuffer = new byte[SocketDisplayer.UDP_BUF_SIZE];
        public int pendingBufOfs = 0;
        public int pendingBufLen = 0;
        public byte[] spriteData = new byte[0];
        public int validDataLen = 0;
        public Image tbCache;
        public AudioClip[] samples = new AudioClip[0];
        public int[] playingSounds = new int[0];

        Playfield(pclient aclient, int width, int height, Color bkgnd) {
            client = aclient;
            pfwidth = width;
            pfheight = height;
            backBuffer = createImage(width, height);
            Graphics backGC = backBuffer.getGraphics();
            backGC.setColor(bkgnd);
            backGC.fillRect(0, 0, width, height);
            backGC.dispose();
            client.resize(width, height);
            client.setBackground(bkgnd);

            int[] pixelData = new int[32*TASKBAR_HEIGHT];
            int target = 0;
            for (int y=0; y<TASKBAR_HEIGHT; y++) {
                int alpha = y * 256 / TASKBAR_HEIGHT;
                int rgb = 0x8080FF | (alpha<<24);
                for (int x=0; x<32; x++)
                    pixelData[target++] = rgb;
            }
            tbCache = createImage(new MemoryImageSource(32, TASKBAR_HEIGHT,
                                                        pixelData, 0, 32));
        }

        public synchronized byte[] setSprites(byte[] buf, int buflen) {
            byte[] old = pendingBuffer;
            pendingBuffer = buf;
            //System.out.println("UDP packet for "+Integer.toString(buflen/6));
            
            /* sound support -- no volume */
            int base = 0;
            int[] currentSounds = new int[samples.length];
            while (base+6 <= buflen &&
                   pendingBuffer[base+4] == -1 &&
                   pendingBuffer[base+5] == -1) {
                int key = pendingBuffer[base+1];
                key = (key & 0xFF) | (((int) pendingBuffer[base]) << 8);
                if (0 <= key && key <= 9999) {   /* safety bound check */
                    if (key >= samples.length) {
                        AudioClip[] newclip = new AudioClip[key+5];
                        System.arraycopy(samples, 0, newclip, 0, samples.length);
                        samples = newclip;
                    }
                    if (samples[key] == null) {
                        String filename = "sample.wav?code=" +
                            Integer.toString(key);
                        samples[key] = getAudioClip(getCodeBase(), filename);
                    }
                    else if (playingSounds.length > key &&
                             playingSounds[key] > 0) {
                        currentSounds[key] = playingSounds[key] - 1;
                    }
                    else {
                        samples[key].play();
                        currentSounds[key] = 4;
                    }
                }
                base += 6;
            }
            playingSounds = currentSounds;
            pendingBufOfs = base;
            pendingBufLen = buflen - base;
            return old;
        }

        public synchronized int fetchSprites() {
            int valid;
            int count = (pendingBufLen < validDataLen) ? pendingBufLen
                : validDataLen;
            
            for (valid=0; valid<count; valid++)
                if (spriteData[valid] != pendingBuffer[pendingBufOfs+valid])
                    break;
            validDataLen = valid;
            
            if (pendingBufLen > spriteData.length) {
                spriteData = new byte[pendingBufLen+90];
                valid = 0;
            }
            System.arraycopy(pendingBuffer, pendingBufOfs+valid,
                             spriteData, valid, pendingBufLen-valid);
            return pendingBufLen;
        }

        public void paint(Graphics g) {
            int buflen = fetchSprites();
            byte[] buffer = spriteData;
            int count = validDataLen / 6;
            int base = count * 6;
            int nspr = buflen / 6;
            Image tback;
            
            if (nspr > sprites.length) {
                Sprite[] newspr = new Sprite[nspr+15];
                System.arraycopy(sprites, 0, newspr, 0, sprites.length);
                for (int i=sprites.length; i<newspr.length; i++)
                    newspr[i] = new Sprite();
                sprites = newspr;
            }

            //System.out.println("drawImage: -"+
            //                   Integer.toString(numSprites-count)+
            //                   " +"+Integer.toString(nspr-count));
            
            // erase extra sprites
            Graphics backGC = backBuffer.getGraphics();
            for (int i=numSprites-1; i>=count; i--) {
                sprites[i].erase(client, backGC);
            }
            
            // draw new sprites
            validDataLen = pendingBufLen;
            while (count < nspr) {
                Sprite s = sprites[count++];
                int x   = buffer[base+1];
                  s.x   = (x   & 0xFF) | (((int) buffer[base  ]) << 8);
                int y   = buffer[base+3];
                  s.y   = (y   & 0xFF) | (((int) buffer[base+2]) << 8);
                int ico = buffer[base+5];
                  s.ico = (ico & 0xFF) | (((int) buffer[base+4]) << 8);
                if (!s.draw(client, backBuffer, backGC)) {
                    if (base < validDataLen)
                        validDataLen = base;
                    //System.out.println(Integer.toString(s.x)+';'+
                    //                   Integer.toString(s.y)+';'+
                    //                   Integer.toString(s.ico));
                }
                base += 6;
            }
            numSprites = count;

            if (client.taskbarmode)
                tback = paintTaskbar(backGC);
            else
                tback = null;

            g.drawImage(backBuffer, 0, 0, client);

            if (tback != null)
                eraseTaskbar(backGC, tback);
            backGC.dispose();
        }

        public Image paintTaskbar(Graphics g) {
            boolean animated = false;
            int y0 = pfheight - TASKBAR_HEIGHT;
            Image bkgnd = client.createImage(pfwidth, TASKBAR_HEIGHT);
            bkgnd.getGraphics().drawImage(backBuffer, 0, -y0, client);
            for (int i=0; i<pfwidth; i+=32)
                g.drawImage(tbCache, i, y0, client);

            double f = 0.0015 * new Date().getTime();
            f = f - (int) f;
            double f2 = f * (1.0-f) * 4.0;

            int lpos = 0;
            int rpos = pfwidth;
            for (Player p=firstPlayer(); p!=null; p=nextPlayer(p))
                if (p.icon != null) {
                    Image ico = p.icon;
                    int x, y;
                    int w = ico.getWidth(client);
                    int h = ico.getHeight(client);
                    int dx = w * 5 / 3;
                    p.xmin = p.xmax = 0;

                    if (p.local) {
                        rpos -= dx;
                        int dy = TASKBAR_HEIGHT - h - 1;
                        x = rpos;
                        y = pfheight - h - (int)(dy*f2);
                        animated = true;
                    }
                    else {
                        lpos += dx;
                        if (p.playing)
                            continue;
                        x = lpos - w;
                        y = pfheight - h;
                        if (keydefinition_k != null &&
                            keydefinition_pid == p.pid) {
                            Image[] icons = keydefinition_k.keyicons;
                            if (icons.length > 0) {
                                int index = (int)(f*icons.length);
                                ico = icons[index % icons.length];
                                animated = true;
                            }
                            y = y0 + (TASKBAR_HEIGHT-h)/2;
                        }
                    }
                    p.xmin = x;
                    p.xmax = x + w;
                    g.drawImage(ico, x, y, client);
                }
            
            if (animated)
                repaint(50);
            return bkgnd;
        }

        public void eraseTaskbar(Graphics g, Image bkgnd) {
            int y0 = pfheight - TASKBAR_HEIGHT;
            g.drawImage(bkgnd, 0, y0, client);
        }
    }

    // Socket listener

    class SocketListener extends Thread {

        public pclient client;
        public Socket socket;
        public InputStream socketInput;
        public OutputStream socketOutput;

        public static final String MSG_WELCOME = "Welcome to gamesrv.py(3) !\n";
        public static final byte MSG_DEF_PLAYFIELD = (byte) 'p';
        public static final byte MSG_DEF_KEY       = (byte) 'k';
        public static final byte MSG_DEF_ICON      = (byte) 'r';
        public static final byte MSG_DEF_BITMAP    = (byte) 'm';
        public static final byte MSG_DEF_SAMPLE    = (byte) 'w';
        public static final byte MSG_DEF_MUSIC     = (byte) 'z';
        public static final byte MSG_PLAY_MUSIC    = (byte) 'Z';
        public static final byte MSG_FADEOUT       = (byte) 'f';
        public static final byte MSG_PLAYER_JOIN   = (byte) '+';
        public static final byte MSG_PLAYER_KILL   = (byte) '-';
        public static final byte MSG_PLAYER_ICON   = (byte) 'i';
        public static final byte MSG_PING          = (byte) 'g';
        public static final byte MSG_PONG          = (byte) 'G';
        public static final byte MSG_INLINE_FRAME  = (byte) '\\';

        public static final byte CMSG_KEY          = (byte) 'k';
        public static final byte CMSG_ADD_PLAYER   = (byte) '+';
        public static final byte CMSG_REMOVE_PLAYER= (byte) '-';
        public static final byte CMSG_UDP_PORT     = (byte) '<';
        public static final byte CMSG_ENABLE_SOUND = (byte) 's';
        public static final byte CMSG_ENABLE_MUSIC = (byte) 'm';
        public static final byte CMSG_PING         = (byte) 'g';
        public static final byte CMSG_PONG         = (byte) 'G';
        public static final byte CMSG_PLAYER_NAME  = (byte) 'n';

        public void connectionClosed() throws IOException {
            throw new IOException("connection closed");
        }

        public void protocolError() throws IOException {
            throw new IOException("protocol error");
        }

        SocketListener(pclient aclient, Socket asocket) throws IOException {
            setDaemon(true);
            byte[] msgWelcome = MSG_WELCOME.getBytes("UTF8");

            client = aclient;
            socket = asocket;
            socketInput = socket.getInputStream();
            socketOutput = socket.getOutputStream();
        
            for (int i=0; i<msgWelcome.length; i++) {
                int recv = socketInput.read();
                if (recv != msgWelcome[i])
                    throw new IOException
                        ("connected to something not a game server");
            }
            showStatus("Let's "+client.readLine(socketInput)+"!");
        }

        public void sendData(byte[] buffer, int ofs, int size)
                                            throws IOException {
            socketOutput.write(buffer, ofs, size);
        }

        public byte[] codeMessage(int p, byte msgcode, int[] args,
                                  String lastarg) {
            int bufsize = 1;
            String mode = "";
            for (int i=0; i<args.length; i++) {
                mode = mode + "l";
                bufsize = bufsize + 4;
            }
            if (lastarg != null) {
                mode = mode + Integer.toString(lastarg.length()) + "s";
                bufsize = bufsize + lastarg.length();
            }
            byte[] buffer = new byte[p + 1 + mode.length() + bufsize];
            buffer[p++] = (byte) mode.length();
            for (int i=0; i<mode.length(); i++) {
                buffer[p++] = (byte) mode.charAt(i);
            }
            buffer[p++] = msgcode;
            for (int i=0; i<args.length; i++) {
                int n;
                int value = args[i];
                buffer[p++] = (byte) (value >> 24);
                n = value >> 16;
                buffer[p++] = (byte)(((n&0x80) == 0) ? n&0x7F : n|0xFFFFFF80);
                n = value >> 8;
                buffer[p++] = (byte)(((n&0x80) == 0) ? n&0x7F : n|0xFFFFFF80);
                n = value;
                buffer[p++] = (byte)(((n&0x80) == 0) ? n&0x7F : n|0xFFFFFF80);
            }
            if (lastarg != null) {
                for (int i=0; i<lastarg.length(); i++) {
                    buffer[p++] = (byte) lastarg.charAt(i);
                }
            }
            return buffer;
        }

        public void sendMessage(byte msgcode, int[] args) throws IOException {
            sendMessageEx(msgcode, args, null);
        }

        public void sendMessageEx(byte msgcode, int[] args, String lastarg)
            throws IOException {
            byte[] buffer = codeMessage(0, msgcode, args, lastarg);
            sendData(buffer, 0, buffer.length);
        }

        public int decodeMessage(byte[] buffer, int ofs, int end)
                                               throws IOException {
            if (ofs == end)   return -1;
            int typecodes = buffer[ofs];  typecodes &= 0xFF;
            int base = ofs+1+typecodes;
            if (base >= end)  return -1;
            byte msgcode = buffer[base++];
            int[] args = new int[typecodes];
            int repeatcount = 0;
            int nargs = 0;
            for (int i=0; i<typecodes; i++) {
                byte c = buffer[ofs+1+i];
                if (c == (byte) 'B') {
                    if (base+1 > end)
                        return -1;
                    args[nargs++] = ((int) buffer[base++]) & 0xFF;
                }
                else if (c == (byte) 'l') {
                    if (base+4 > end)
                        return -1;
                    int n4 = buffer[base++];
                    int n3 = buffer[base++];  n3 &= 0xFF;
                    int n2 = buffer[base++];  n2 &= 0xFF;
                    int n1 = buffer[base++];  n1 &= 0xFF;
                    int value = n1 | (n2<<8) | (n3<<16) | (n4<<24);
                    //System.out.println(n4);
                    //System.out.println(n3);
                    //System.out.println(n2);
                    //System.out.println(n1);
                    //System.out.println(value);
                    //System.out.println();
                    args[nargs++] = value;
                }
                else if ((byte) '0' <= c && c <= (byte) '9') {
                    repeatcount = repeatcount*10 + (c - (byte) '0');
                }
                else if (c == (byte) 's') {
                    if (base+repeatcount > end)
                        return -1;
                    args[nargs++] = base;
                    args[nargs++] = repeatcount;
                    base += repeatcount;
                    repeatcount = 0;
                }
                else
                    protocolError();
            }

            //System.out.print("Message ");
            //System.out.print((char) msgcode);
            //for (int i=0; i<nargs; i++)
            //    System.out.print(" " + Integer.toString(args[i]));
            //System.out.println();
            
            switch (msgcode) {

            case MSG_PLAYER_JOIN: {
                int id = args[0];
                int local = args[1];
                Player p = client.getPlayer(id);
                p.playing = true;
                p.local = local != 0;
                if (p.local)
                    client.setTaskbar(false);
                break;
            }
            case MSG_PLAYER_KILL: {
                int id = args[0];
                Hashtable nkeycodes = new Hashtable();
                Player p = client.getPlayer(id);
                p.playing = false;
                p.local = false;
                for (Enumeration e = client.keycodes.keys();
                     e.hasMoreElements(); ) {
                    Object key = e.nextElement();
                    byte[] msg = (byte[]) keycodes.get(key);
                    if (msg[0] != id)
                        nkeycodes.put(key, msg);
                }
                client.keycodes = nkeycodes;
                break;
            }
            case MSG_DEF_PLAYFIELD: {
                int width = args[0];
                int height = args[1];
                Color bkgnd = client.makeColor(args[2]);
                client.playfield = new pclient.Playfield(client, width, height,
                                                         bkgnd);
                int[] singleint = new int[1];
                singleint[0] = -1;
                sendMessage(CMSG_ENABLE_SOUND, singleint);
                sendMessage(CMSG_PING, new int[0]);
                break;
            }
            case MSG_DEF_KEY: {
                int i;
                int nameofs = args[0];
                int namelen = args[1];
                int keyid   = args[2];
                int nicons  = nargs - 3;
                KeyName key = new KeyName();
                key.keyname = new String(buffer, nameofs, namelen, "UTF8");
                key.keyid   = keyid;
                key.keyicons= new Image[nicons];
                for (i=0; i<nicons; i++)
                    key.keyicons[i] = client.getIcon(args[3+i]);
                if (client.keys == null || keyid < client.keys.keyid) {
                    key.next = client.keys;
                    client.keys = key;
                }
                else {
                    KeyName k = client.keys;
                    while (k.next != null && k.next.keyid < keyid)
                        k = k.next;
                    key.next = k.next;
                    k.next = key;
                }
                break;
            }
            case MSG_DEF_ICON: {
                int bmpcode = args[0];
                int icocode = args[1];
                int ix = args[2];
                int iy = args[3];
                int iw = args[4];
                int ih = args[5];
                if (bmpcode < client.bitmaps.length) {
                    Bitmap bmp = client.bitmaps[bmpcode];
                    if (bmp != null)
                        client.setIcon(icocode,
                                       bmp.extractIcon(ix, iy, iw, ih));
                }
                break;
            }
            case MSG_DEF_BITMAP: {
                int bmpcode = args[0];
                int dataofs = args[1];
                int datalen = args[2];
                int colorkey = (nargs > 3) ? args[3] : -1;
                InputStream st = decompresser(buffer, dataofs, datalen);
                client.setBitmap(bmpcode, new Bitmap(client, st, colorkey));
                break;
            }
            case MSG_PLAYER_ICON: {
                int pid = args[0];
                int icocode = args[1];
                Player p = client.getPlayer(pid);
                p.icon = client.getIcon(icocode);
                break;
            }
            case MSG_PING: {
                buffer[ofs+1+typecodes] = CMSG_PONG;
                sendData(buffer, ofs, base-ofs);
                if (nargs > 0 && !client.udpovertcp) {
                    int udpkbytes = args[0];
                    /* switch to udp_over_tcp if the udp socket didn't
                       receive at least 60% of the packets sent by the server,
                       or if the socketdisplayer thread died */
                    if (sockdisplayer != null && !sockdisplayer.isAlive()) {
                        showStatus("routing UDP traffic over TCP (no UDP socket)");
                        client.start_udp_over_tcp();
                    }
                    else if (udpkbytes * 1024.0 * 0.60 > client.udpbytecounter) {
                        client.udpsock_low += 1;
                        if (client.udpsock_low >= 4) {
                            double inp =client.udpbytecounter/(udpkbytes*1024.0);
                            int loss = (int)(100.0*(1.0-inp));
                            showStatus("routing UDP traffic over TCP (" +
                                       Integer.toString(loss) +
                                       "% packet loss)");
                            client.start_udp_over_tcp();
                        }
                    }
                    else
                        client.udpsock_low = 0;
                }
                break;
            }
            case MSG_PONG: {
                if (!client.taskbarfree && !client.taskbarmode) {
                    client.taskbarfree = true;
                    client.setTaskbar(true);
                }
                break;
            }
            case MSG_INLINE_FRAME: {
                if (client.uinflater != null) {
                    int dataofs = args[0];
                    int datalen = args[1];
                    int len;
                    byte[] pkt = client.uinflater_buffer;
                    client.uinflater.setInput(buffer, dataofs, datalen);
                    try {
                        len = client.uinflater.inflate(pkt);
                    }
                    catch (DataFormatException e) {
                        len = 0;
                    }
                    Playfield pf = client.playfield;
                    if (len > 0 && pf != null) {
                        client.uinflater_buffer = pf.setSprites(pkt, len);
                        client.repaint();
                    }
                }
                break;
            }
            default: {
                System.err.println("Note: unknown message " +
                                   Byte.toString(msgcode));
                break;
            }
            }
            return base;
        }

        public void run() {
            try {
                byte[] buffer = new byte[0xC000];
                int begin = 0;
                int end = 0;
                try {
                    while (true) {
                        if (end + 0x6000 > buffer.length) {
                            // compact buffer
                            byte[] newbuf;
                            end = end-begin;
                            if (end + 0x8000 > buffer.length)
                                newbuf = new byte[end + 0x8000];
                            else
                                newbuf = buffer;
                            System.arraycopy(buffer, begin, newbuf, 0, end);
                            begin = 0;
                            buffer = newbuf;
                        }
                        int count = socketInput.read(buffer, end, 0x6000);
                        if (isInterrupted())
                            break;
                        if (count <= 0)
                            connectionClosed();
                        end += count;
                        while ((count=decodeMessage(buffer, begin, end)) >= 0) {
                            begin = count;
                        }
                    }
                }
                catch (InterruptedIOException e) {
                }
                socket.close();
            }
            catch (IOException e) {
                client.debug(e);
            }
        }
    }

    // UDP Socket messages

    class SocketDisplayer extends Thread {
        public static final int UDP_BUF_SIZE = 0x10000;

        public pclient client;
        public DatagramSocket socket;

        SocketDisplayer(pclient aclient) {
            setDaemon(true);
            client = aclient;
        }

        public void run() {
            /* This thread may die early, typically because of JVM
               security restrictions. */
            byte[] buffer = new byte[UDP_BUF_SIZE];
            DatagramPacket pkt = new DatagramPacket(buffer, UDP_BUF_SIZE);
            try {
                {
                    socket = new DatagramSocket();
                    int[] args = new int[1];
                    args[0] = socket.getLocalPort();
                    client.socklistener.sendMessage(SocketListener.CMSG_UDP_PORT,
                                                    args);
                }
                try {
                    while (true) {
                        socket.receive(pkt);
                        if (isInterrupted())
                            break;
                        client.udpbytecounter += (double) pkt.getLength();
                        Playfield pf = client.playfield;
                        if (pf != null) {
                            pkt.setData(pf.setSprites(pkt.getData(),
                                                      pkt.getLength()));
                            pkt.setLength(UDP_BUF_SIZE);
                            client.repaint();
                        }
                    }
                }
                catch (InterruptedIOException e) {
                }
                socket.close();
            }
            catch (IOException e) {
                client.debug(e);
            }
        }
    }

    // Applet methods

    public SocketListener socklistener = null;
    public SocketDisplayer sockdisplayer = null;
    public Playfield playfield = null;

    public void init() {
        try {
            Socket link;
            String param;

            String gamesrv = getParameter("gamesrv");
            if (gamesrv == null) {
                gamesrv = getDocumentBase().getHost();
            }
            param = getParameter("gameport");
            if (param != null) {
                // direct TCP connexion to the game server
                link = new Socket(gamesrv, Integer.parseInt(param));
            }
            else {
                // UCP query
                param = getParameter("port");
                int port = (param != null) ? Integer.parseInt(param) : defaultPort;
                link = pickHost(gamesrv, port);
            }
            socklistener = new SocketListener(this, link);
            socklistener.start();
        }
        catch (IOException e) {
            debug(e);
        }
    }

    public void destroy() {
        if (socklistener != null) {
            socklistener.interrupt();
            socklistener = null;
        }
    }

    public void start() {
        enableEvents(AWTEvent.KEY_EVENT_MASK |
                     AWTEvent.MOUSE_EVENT_MASK |
                     AWTEvent.MOUSE_MOTION_EVENT_MASK);
        if (socklistener != null) {
            sockdisplayer = new SocketDisplayer(this);
            sockdisplayer.start();
        }
    }

    public void stop() {
        if (sockdisplayer != null) {
            sockdisplayer.interrupt();
            sockdisplayer = null;
        }
    }

    public void update(Graphics g) {
        paint(g);
    }

    public void paint(Graphics g) {
        Playfield pf = playfield;
        if (pf != null) {
            pf.paint(g);
        }
        else {
            int appWidth = getSize().width;
            int appHeight = getSize().height;
            g.clearRect(0, 0, appWidth, appHeight);
        }
    }
    
    protected void processKeyEvent(KeyEvent e) {
        int num;
        byte[] msg;
        e.consume();
        switch (e.getID()) {
        case KeyEvent.KEY_PRESSED:
            num = e.getKeyCode();
            break;
        case KeyEvent.KEY_RELEASED:
            num = -e.getKeyCode();
            break;
        default:
            return;
        }
        msg = (byte[]) keycodes.get(new Integer(num));
        if (msg != null && socklistener != null) {
            Player p = getPlayer(msg[0]);
            if (p.local) {
                try {
                    socklistener.sendData(msg, 1, msg.length-1);
                }
                catch (IOException ioe) {
                    debug(ioe);
                }
                return;
            }
        }
        if (keydefinition_k != null && e.getID() == KeyEvent.KEY_PRESSED)
            defineKey(num);
    }

    public void nextKey() {
        KeyName k = keydefinition_k;
        if (k == null)
            k = keys;
        else
            k = k.next;
        while (k != null && k.keyname.charAt(0) == '-') {
            k.newkeycode = 0;
            k = k.next;
        }
        keydefinition_k = k;
    }

    public void defineKey(int num) {
        KeyName k;
        for (k=keys; k!=keydefinition_k; k=k.next)
            if (k.newkeycode == num)
                return;
        k.newkeycode = num;
        nextKey();
        if (keydefinition_k == null) {
            if (socklistener != null) {
                try {
                    byte[] buffer;
                    int[] args = new int[1];
                    args[0] = keydefinition_pid;
                    socklistener.sendMessage(SocketListener.CMSG_ADD_PLAYER,
                                             args);
                    String param = "player" +
                        Integer.toString(keydefinition_pid);
                    param = getParameter(param);
                    if (param != null) {
                        socklistener.sendMessageEx(
				SocketListener.CMSG_PLAYER_NAME,
                                args,
                                param);
                    }
                    args = new int[2];
                    args[0] = keydefinition_pid;
                    for (k=keys; k!=null; k=k.next) {
                        if (k.keyname.charAt(0) == '-') {
                            String test = k.keyname.substring(1);
                            for (KeyName r=keys; r!=null; r=r.next)
                                if (r.keyname.equals(test))
                                    k.newkeycode = -r.newkeycode;
                        }
                        args[1] = k.keyid;
                        buffer = socklistener.codeMessage
                            (1, SocketListener.CMSG_KEY, args, null);
                        buffer[0] = (byte) keydefinition_pid;
                        keycodes.put(new Integer(k.newkeycode), buffer);
                    }
                }
                catch (IOException ioe) {
                    debug(ioe);
                }
            }
        }
        repaint();
    }

    protected void processMouseMotionEvent(MouseEvent e) {
        Playfield pf = playfield;
        if (pf != null)
            setTaskbar(e.getY() >= pf.pfheight - pf.TASKBAR_HEIGHT);
        e.consume();
    }

    protected void processMouseEvent(MouseEvent e) {
        if (e.getID() != MouseEvent.MOUSE_PRESSED) {
            e.consume();
            return;
        }
        requestFocus();
        keydefinition_k = null;
        Playfield pf = playfield;
        if (pf != null && e.getY() >= pf.pfheight - pf.TASKBAR_HEIGHT) {
            int x = e.getX();
            for (Player p=firstPlayer(); p!=null; p=nextPlayer(p))
                if (p.xmin <= x && x < p.xmax) {
                    if (p.local) {
                        if (socklistener != null) {
                            try {
                                int[] args = new int[1];
                                args[0] = p.pid;
                                socklistener.sendMessage
                                    (SocketListener.CMSG_REMOVE_PLAYER, args);
                            }
                            catch (IOException ioe) {
                                debug(ioe);
                            }
                        }
                    }
                    else {
                        keydefinition_pid = p.pid;
                        nextKey();
                    }
                    break;
                }
        }
        e.consume();
        repaint();
    }

    // UDP-over-TCP
    public double udpbytecounter = 0.0;
    public int udpsock_low = 0;
    public boolean udpovertcp = false;
    public Inflater uinflater = null;
        public byte[] uinflater_buffer = null;

    public void start_udp_over_tcp()
    {
        udpovertcp = true;
        int[] args = new int[1];
        args[0] = 0;
        try {
            socklistener.sendMessage(SocketListener.CMSG_UDP_PORT, args);
        }
        catch (IOException e) {
            return;
        }
        uinflater_buffer = new byte[SocketDisplayer.UDP_BUF_SIZE];
        uinflater = new Inflater();
        if (sockdisplayer != null) {
            sockdisplayer.interrupt();
            sockdisplayer = null;
        }
    }

//     // ImageObserver interface

//     public boolean imageUpdate(Image img,
//                                int infoflags,
//                                int x,
//                                int y,
//                                int width,
//                                int height) {
//         return false;
//     }
}
