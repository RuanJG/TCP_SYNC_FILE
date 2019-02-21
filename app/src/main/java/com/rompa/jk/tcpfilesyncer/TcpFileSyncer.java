package com.rompa.jk.tcpfilesyncer;
import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.UnknownHostException;
import java.security.MessageDigest;

/**
 * Created by RSL-030 on 2018/11/7.
 */

public class TcpFileSyncer {
    private Context mContext = null;
    private Handler mHandler = null;
    private Socket mSocket = null;
    private Thread mTcpThreader = null;
    private volatile boolean mTcpThreadStop = true;
    private volatile boolean mTcpForceReconnect = false;
    private DataInputStream mDataInputStream = null;
    private String mSaveFileName = "Sleeve_App_BLE_Data/SBS_MP_Data.txt";//“内部存储”目录下的路径
    private SerialCoder mCoder = new SerialCoder();

    public String mId = "5";  // difference pad use ID0 , ID1 , ID2 ...
    public String mType = "TYPE3"; // fixed
    public String mIP = "127.0.0.1";
    public int mPort = 55555;

    int fileFrameSize = 256;

    final int MD5_CODE_TAG = -1;
    final int FILE_SIZE_TAG = -2;
    class ACKType {
        final  static int OK =0;
        final  static int ERROR_ID =1;
        final  static int ERROR_TYPE=2;
        final  static int ERROR_SETUP_ID_TYPE=3;
        final  static int ERROR_DATA=4;
        final  static int ERROR_REPEAT_BARCODE=5;
        final  static int ERROR_FILE=6;
    }

    public TcpFileSyncer(Context context,Handler feedbackHandler,String filepath, String ip, int port, String id)
    {
        mContext = context;
        mHandler = feedbackHandler;
        mSaveFileName = filepath;
        mIP = ip;
        mPort = port;
        mId = id;
    }
    public void finalize(){
        Logout();
    }

    public void Login()
    {
        if( !isTcpConnected() )
            startTcpThread();
    }
    public void Logout()
    {
        stopTcpThread();
    }
    public void Reconnect(){
        mTcpForceReconnect = true;
    }

    public boolean isTcpConnected()
    {
        return ( mSocket!=null && mSocket.isConnected() );
        //return ( mSocket!=null );
    }


    private  void startTcpThread()
    {
        if( mTcpThreader != null) {
            stopTcpThread();
        }
        mTcpThreader = null;
        mTcpThreadStop = false;
        mTcpForceReconnect = false;
        mTcpThreader = new Thread(tcpThreadRunner);
        mTcpThreader.start();
        mCoder.clear();
    }

    private  void stopTcpThread() {
        if( mTcpThreader == null)
            return;
        mTcpThreader.interrupt();
        mTcpThreadStop = true;
        mTcpThreader = null;
        //while (mTcpThreader.isAlive());
    }

    Runnable tcpThreadRunner = new Runnable() {
        @Override
        public void run() {
            int timerMs = 0;
            mTcpThreadStop = false;
            boolean logined = false;
            try {
                while (!mTcpThreadStop)
                {
                    if( timerMs <= 0 ) {
                        if( !isTcpConnected() ) {
                            logined = false;
                            timerMs = 2*1000;
                            if( connectTCP() ) {
                                timerMs = 500;
                            }
                        }else if( !logined ){
                            logined = LoginLoop();
                            timerMs = logined? (1*1000):(2*1000);
                        }else{
                            syncLoop();
                            timerMs = 5*1000; // each timerMs sync one time;
                        }
                        //TODO check TCP is connecting
                        //Now just check whether send bytes , if send failed , close tcp and reconnect in this loop
                    }
                    if(  mTcpForceReconnect ){
                        //change ip setting , reconnect
                        disconnectTCP();
                        timerMs = 500;
                        mTcpForceReconnect = false;
                    }

                    Thread.sleep(1);
                    if( timerMs > 0 ) timerMs--;
                }
            } catch (InterruptedException e) {
                Log.e("ruan","tcpreciveRunner get interrupted");
            }
            Log.e("ruan","tcpreciveRunner exit");
            disconnectTCP();
        }
    };

    private boolean connectTCP()
    {
        Log.e("Ruan: ", "connectTCP");

        if( mSocket !=null ) {
            disconnectTCP();
        }
        try {
            threadLog("try to connect "+mIP+":"+mPort+"\n");
            mSocket = new Socket();
            SocketAddress socAddress;
            InetAddress ia = InetAddress.getByName(mIP);
            socAddress = new InetSocketAddress(ia.getHostAddress(), mPort);
//            ia.isAnyLocalAddress()
//            if( ia.getHostAddress().equals(mIP) ){
//                //mIP is ip addresss
//                threadLog("is ip address\n"+ia.getHostAddress());
//                 socAddress = new InetSocketAddress(mIP, mPort);
//            }else{
//                //mIP is hostname
//                threadLog("is hostname\n"+ia.getHostAddress());
//                 socAddress = new InetSocketAddress(ia, mPort);
//            }
            mSocket.connect(socAddress, 2000);//block until timeout, if false will catch exception
            if( mSocket.isConnected()) {
                threadLog("connected!!\n");
                mSocket.setSoTimeout(2000);
                mDataInputStream = new DataInputStream(mSocket.getInputStream());
                return true;
            }else {
                Log.e("Ruan: ", "unknow connect false !!");
            }
        } catch (UnknownHostException e) {
            e.printStackTrace();
            Log.e("Ruan: ", "Connect Failed , Unknow Host!!");
        } catch (IOException e) {
            e.printStackTrace();
            Log.e("Ruan: ", "Connect Failed , IO error!!");
        }
        disconnectTCP();
        return false;
    }

    private void disconnectTCP()
    {
        Log.e("Ruan: ", "disconnectTCP");
        mDataInputStream = null;
        if( mSocket !=null) {
            if (mSocket.isConnected()) {
                try {
                    Log.e("Ruan: ", "close socket");
                    mSocket.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        mSocket = null;
        //threadLog("tcp diconnect \n");
    }


    private boolean LoginLoop()
    {
        String msg;
        sendTcpBytes(("ID"+mId).getBytes());
        if( ! listenAck(1000) ) {
            threadLog("ID certify false\n");
            return false;
        }
        sendTcpBytes( mType.getBytes() );
        if(  ! listenAck(1000)  ) {
            threadLog("TYPE certify false\n");
            return false;
        }
        threadLog("Login successfully\n");
        return true;
    }

    private void syncLoop( )
    {
        //msg = [tag][#][data]
        //send md5
        String md5str = getExternalFileMD5(mContext, mSaveFileName);
        Log.e("Ruan","MD5="+md5str);
        //byte[] tagbytes = intToQByteArray(MD5_CODE_TAG);
        String tagStr = String.valueOf(MD5_CODE_TAG)+"#";
        byte[] tagbytes =tagStr.getBytes();
        byte[] msg = new byte[md5str.length()+tagbytes.length];
        System.arraycopy(tagbytes,0, msg,0, tagbytes.length);
        System.arraycopy(md5str.getBytes(), 0 , msg, tagbytes.length, md5str.length());
        sendTcpBytes(msg);
        if( listenAck(3000) ) {
            //threadLog("Sync OK");
            return;//same md5, don't need to sync
        }

        //get file size
        //tagbytes = intToQByteArray(FILE_SIZE_TAG);
        tagStr = String.valueOf(FILE_SIZE_TAG)+"#";
        tagbytes = tagStr.getBytes();
        String sizeStr = String.valueOf(getFileSize(mContext, mSaveFileName));
        msg = new byte[sizeStr.length()+tagbytes.length];
        System.arraycopy(tagbytes,0, msg,0, tagbytes.length);
        System.arraycopy(sizeStr.getBytes(), 0 , msg, tagbytes.length, sizeStr.length());
        sendTcpBytes(msg);
        byte[] filesizebyte = listenTcpOneMsg(5000);
        long serverFileSize =0;

        if( filesizebyte.length == 8 ){
            int t = QByteArrayToInt( getSubByte(filesizebyte,0,4) );
            serverFileSize = QByteArrayToInt( getSubByte(filesizebyte,4,4) );
            if( t != FILE_SIZE_TAG ){
                threadLog("Sync Data False：get service file size false 1 \n");
                return;
            }
            if( serverFileSize > getFileSize(mContext, mSaveFileName)){
                threadLog("Sync Data False：To backup the data file in the Service first !!!!\n");
                threadNotify("Sync Data False：To backup the data file in the Service first !!!!");
                //mTcpThreadStop = true;
                return;
            }
            //goto sync file
        }else{
            threadLog("Sync Data False：get service file size false 2\n");
            return;
        }

        // sync file
        long totalLen = (int) getFileSize(mContext, mSaveFileName);
        long currentPos = serverFileSize<totalLen ? serverFileSize:0;


        if( currentPos == 0 )threadLog("Start sync file ....\n");
        else threadLog("Appending data ....\n");

        byte[] fdata;
        while(true)
        {
            threadLog(String.valueOf(currentPos)+"/"+String.valueOf(totalLen)+" = "+ 100*currentPos/totalLen + "%\n");
            fdata = readExternalFileBytes(mContext, mSaveFileName,currentPos,fileFrameSize);
            if( fdata.length > 0){
                //tagbytes = intToQByteArray(currentPos);
                tagStr = String.valueOf(currentPos)+"#";
                tagbytes = tagStr.getBytes();
                msg = new byte[tagbytes.length+fdata.length];
                System.arraycopy(tagbytes,0, msg,0, tagbytes.length);
                System.arraycopy(fdata, 0 , msg, tagbytes.length, fdata.length);
                sendTcpBytes(msg);
                if(  ! listenAck(3000) ) {
                    threadLog("Sync Data False：Ack timeout\n");
                    return;
                }
                currentPos += fdata.length;
                if( currentPos >= totalLen) break;
            }else{
                // finish sync
                break;
            }
        }

        //recheck md5
        md5str = getExternalFileMD5(mContext, mSaveFileName);
        //byte[] tagbytes = intToQByteArray(MD5_CODE_TAG);
        tagStr = String.valueOf(MD5_CODE_TAG)+"#";
        tagbytes =tagStr.getBytes();
        msg = new byte[md5str.length()+tagbytes.length];
        System.arraycopy(tagbytes,0, msg,0, tagbytes.length);
        System.arraycopy(md5str.getBytes(), 0 , msg, tagbytes.length, md5str.length());
        sendTcpBytes(msg);
        if( listenAck(3000) ) {
            threadLog("Sync OK");
        }else{
            threadLog("Sync false");
        }
    }


    private boolean listenAck(int timeoutMs)
    {
        byte[] res = listenTcpOneMsg(timeoutMs);
        if ( res.length > 0 )
        {
            try {
                String restr = new String(res, "utf-8");
                Log.e("Ruan", "ACK=" + restr);
                if (restr.equals("ACK0"))
                    return true;
            } catch (Exception e) {
            }
        }
        return false;
    }

    private byte[] listenTcpOneMsg(int timeoutMs)
    {
        byte[] retByte = new byte[1] ;
        byte[] pkg;
        int len,ms=0;
        try {
            while(true)
            {
                len = mDataInputStream.available();
                if( len > 0 )
                {
                    for( int i=0; i< len ; i++)
                    {
                        mDataInputStream.read(retByte, 0, 1);
                        pkg  = mCoder.parse(retByte[0],true);
                        if ( pkg.length > 0){
                            return pkg;
                        }
                    }
                }
                Thread.sleep(1);
                ms++;
                if( timeoutMs <= ms ) {
                    threadLog("Receive Timeout!!\n");
                    break;
                }
            }
        }catch (Exception e) {
            // TODO: handle exception
            Log.e("Ruan:", e.toString());
        }
        return new byte[0];
    }

    private byte[] listenTcpBytes(int replyPkgLen, int timeoutMs)
    {
        byte[] retByte = new byte[replyPkgLen];
        byte[] recData;
        int cpcount,len,index=0,ms=0;
        try {
            while(true)
            {
                len = mDataInputStream.available();
                if( len >= replyPkgLen){
                    mDataInputStream.read(retByte);
                    //Log.e("Ruan","len="+len);
                    return retByte;
                }
                Thread.sleep(1);
                ms++;
                if( timeoutMs <= ms ) {
                    threadLog("Receive Timeout!!\n");
                    break;
                }
            }
        }catch (Exception e) {
            // TODO: handle exception
            Log.e("Ruan:", e.toString());
        }
        return new byte[0];
    }

    private  boolean sendTcpBytes(byte[] data)
    {
        byte[] pkg;

        if( mSocket ==null || !mSocket.isConnected()){
            return false;
        }
        try
        {
            pkg = mCoder.encode(data,data.length,true);
            OutputStream outputStream = mSocket.getOutputStream();
            outputStream.write(pkg);
            outputStream.flush();
        }
        catch (Exception e)
        {
            // TODO: handle exception
            Log.e("Ruan:", e.toString());
            threadLog("Server disconnected...\n");
            disconnectTCP();
            return false;
        }
        return true;
    }






    private String getExternalFileMD5(Context context, String filename) {
        String md5str = "";
        if(Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)){
            String fpath = Environment.getExternalStorageDirectory().getPath()+ File.separator + filename;//"/Sleeve_App_BLE_Data" + "/" + "SBS_MP_Data" + ".txt";
            //fpath = context.getExternalCacheDir().getAbsolutePath() + File.separator + filename;

            //打开文件输入流
            md5str = cailFileMD5(fpath);
        }
        return md5str;
    }

    private long getFileSize(Context context, String filename)
    {
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
        {
            String fpath = Environment.getExternalStorageDirectory().getPath()+ File.separator + filename;//"/Sleeve_App_BLE_Data" + "/" + "SBS_MP_Data" + ".txt";
            //fpath = context.getExternalCacheDir().getAbsolutePath() + File.separator + filename;

            File f= new File(fpath);
            if( f.exists() && f.isFile())
                return f.length();
        }else{
            threadLog("SD Card not load\n");
        }
        return 0;
    }

    private  byte[] readExternalFileBytes(Context context, String filename, long pos , int length)
    {
        try
        {
            if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
            {
                String fpath = Environment.getExternalStorageDirectory().getPath()+ File.separator + filename;//"/Sleeve_App_BLE_Data" + "/" + "SBS_MP_Data" + ".txt";
                //fpath = context.getExternalCacheDir().getAbsolutePath() + File.separator + filename;

                //打开文件输入流
                FileInputStream inputStream = new FileInputStream(fpath);
                inputStream.getChannel().position(pos);

                byte[] buffer = new byte[length];
                int len = inputStream.read(buffer);
                byte[] data = new byte[len];
                System.arraycopy(buffer,0,data,0,len);

                //关闭输入流
                inputStream.close();

                return data;
            }
        }catch (Exception e) {
            Log.e("Ruan","error in read file");
            e.printStackTrace();
        }
        return new byte[0];
    }

    private  boolean writeExternalFileBytes(Context context, String filename, int pos, byte[] bytes)
    {
        try
        {
            if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
            {
                String fpath = Environment.getExternalStorageDirectory().getPath()+ File.separator + filename;//"/Sleeve_App_BLE_Data" + "/" + "SBS_MP_Data" + ".txt";
                //fpath = context.getExternalCacheDir().getAbsolutePath() + File.separator + filename;

                //打开文件输入流
                FileOutputStream outputStream = new FileOutputStream(fpath);
                outputStream.getChannel().position(pos);
                outputStream.write(bytes);
                outputStream.close();
                return true;
            }
            Log.e("Ruan","Could not get External Storage");
            return false;
        }catch (Exception e) {
            Log.e("Ruan","error in read file");
            e.printStackTrace();
            return false;
        }
    }

    private void threadLog(String msg)
    {
        if(msg != null){
            Message mesg = new Message();
            Bundle bun = new Bundle();
            bun.putString("log",msg);
            mesg.setData(bun);
            mHandler.sendMessage(mesg);
            Log.e("ruan","threadLog: "+msg);
        }
    }
    private void threadNotify(String msg)
    {
        if(msg != null){
            Message mesg = new Message();
            Bundle bun = new Bundle();
            bun.putString("notify",msg);
            mesg.setData(bun);
            mHandler.sendMessage(mesg);
            Log.e("ruan","threadnotify: "+msg);
        }
    }





    public  static String cailFileMD5(String filename) {
        File file = new File( filename );
        if (!file.isFile()) {
            return null;
        }
        MessageDigest digest = null;
        FileInputStream in = null;
        byte buffer[] = new byte[1024];
        int len;
        try {
            digest = MessageDigest.getInstance("MD5");
            in = new FileInputStream(file);
            while ((len = in.read(buffer, 0, 1024)) != -1) {
                digest.update(buffer, 0, len);
            }
            in.close();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        return bytesToHexString(digest.digest());
    }

    public static byte[] getSubByte(byte[]data,int startPos ,int len){
        byte[] ret=new byte[len];
        for(int i=0;i<len;i++){
            ret[i]=data[startPos+i];
        }
        return ret;
    }

    public static  byte[] intToQByteArray(int i)
    {
        byte[] abyte0 = new byte[4];
        abyte0[0] = (byte)(0x000000ff & i);
        abyte0[1] = (byte) ((0x0000ff00 & i) >> 8);
        abyte0[2] = (byte) ((0x00ff0000 & i) >> 16);
        abyte0[3] = (byte) ((0xff000000 & i) >> 24);
        return abyte0;
    }

    public static int QByteArrayToInt(byte[] data)
    {
        int addr = data[0] & 0x000000FF;
        addr |= ((data[1] << 8) & 0x0000FF00);
        addr |= ((data[2] << 16) & 0x00FF0000);
        addr |= ((data[3] << 24) & 0xFF000000);
        return addr;
    }

    public static String bytesToHexString(byte[] src) {
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv);
        }
        return stringBuilder.toString();
    }

}
