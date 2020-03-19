import java.io.IOException;
        import java.io.PrintWriter;
        import java.net.Socket;
        import java.util.Scanner;

class Countsocket extends Thread{
    public void run(){
        Socket socket = null;//与服务器链接的套接字
        Scanner fromKey = null;//键盘输入流
        Scanner fromServer = null;//获取服务器发送的字节流
        PrintWriter toServer = null;//向服务器发送的字节流

        try {
            //链接服务器的套接字
            socket = new Socket("localhost",8866);

            /*
             * 注意:服务器端与客户端不能都先获取输入流,这样会导致socket阻塞
             * */
            //实例化从服务器读的字节流
            fromServer = new Scanner(socket.getInputStream());
            //实例化向服务器写的字节流
            toServer = new PrintWriter(socket.getOutputStream());
            //实例化键盘输入流
            fromKey = new Scanner(System.in);

            /*将公式中五个参数和ID放到同一个字符串中。发送到计算端
             * 为了方便计算端读取字符串
             * 五个参数之间以 # 隔开
             * 参数与ID之间以 * 隔开
             * ID最后以 * 结尾
             * */
            //公式中的五个参数
            //System.out.print("m_a:");
            String m_a = "0.8#";//fromKey.nextLine();//对应0.8
            //System.out.print("m_b:");
            String m_b = "15#";//fromKey.nextLine();//对应15
            //System.out.print("m_c:");
            String m_c = "10#";//fromKey.nextLine();//对应10
            //System.out.print("p_a:");
            String p_a = "150000#";//fromKey.nextLine();//对应150000
            //System.out.print("p_b:");
            String p_b = "50000#*";//fromKey.nextLine();//对应50000
            //用户ID
            System.out.print("请输入用户ID:");
            String ID = fromKey.nextLine();
            ID += '*';
            //参数发送到计算器端
            String toServerData = m_a + m_b + m_c + p_a + p_b + ID;//融合
            toServer.println(toServerData);
            toServer.flush();

            System.out.println("等待接收");
            String result;
            result = fromServer.nextLine();
            System.out.println(result);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }finally {
            try {
                fromServer.close();
                toServer.close();
                socket.close();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }
}

public class client {
    public static void main(String[] args) {
        new Countsocket().run();
    }

}
