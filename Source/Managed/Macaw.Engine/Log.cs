using System.Text;

namespace Macaw;

/// <summary>엔진 로그에 메시지를 기록합니다.</summary>
public static unsafe class Log {
    internal static NativeApi* sApi;

    public static void Info(string Message) {
        byte[] Bytes = Encoding.UTF8.GetBytes(Message + "\0");

        fixed (byte* Pointer = Bytes) {
            sApi->Log(Pointer);
        }
    }
}
