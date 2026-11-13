using System.Text;

namespace Macaw;

/// <summary>엔진 로그에 메시지를 기록합니다.</summary>
public static unsafe class Log {

    /// <summary>엔진 로그 함수가 들어 있는 외부 소유 API 테이블입니다.</summary>
    internal static NativeApi* sApi;

    /// <summary>메시지를 NUL로 끝나는 UTF-8 버퍼로 변환하여 엔진 로그에 전달합니다.</summary>
    public static void Info(string Message) {
        byte[] Bytes = Encoding.UTF8.GetBytes(Message + "\0");

        fixed (byte* Pointer = Bytes) {
            sApi->Log(Pointer);
        }
    }
}
