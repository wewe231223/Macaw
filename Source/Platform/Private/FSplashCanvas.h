/**
 * @file FSplashCanvas.h
 * @brief 스플래시 창의 배경, 아이콘, 글자와 진행 막대를 DIB 프레임에 그립니다.
 * @ingroup Platform
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <Windows.h>
#include <array>
#include <string>
#include <vector>

namespace Platform {

    /**
     * @brief 스플래시 스레드가 소유하는 DIB 프레임과 글꼴, 미리 합성한 배경입니다.
     * @ingroup Platform
     */
    class FSplashCanvas {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FSplashCanvas);

    public:
        /**
         * @brief 0~1 범위의 선형 RGB 색입니다.
         */
        using FColor = std::array<float, 3>;

    public:
        /**
         * @brief 자원이 없는 빈 캔버스를 구성합니다.
         */
        FSplashCanvas();

        /**
         * @brief DIB와 글꼴을 해제합니다.
         */
        ~FSplashCanvas();

        /**
         * @brief GDI 핸들을 소유하므로 복사를 금지합니다.
         */
        FSplashCanvas(const Platform::FSplashCanvas&) = delete;

        /**
         * @brief GDI 핸들을 소유하므로 대입을 금지합니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Platform::FSplashCanvas& operator=(const Platform::FSplashCanvas&) = delete;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSplashCanvas);

        /**
         * @brief 프레임 DIB와 글꼴을 만들고 고정 배경을 합성합니다.
         * @param[in] Width 프레임의 가로 픽셀 수입니다.
         * @param[in] Height 프레임의 세로 픽셀 수입니다.
         * @param[in] Scale 96 DPI 기준 화면 배율입니다.
         * @param[in] ProjectName 배경에 새길 프로젝트 이름입니다.
         * @return 자원을 모두 만들면 true입니다.
         */
        bool Initialize(Core::Int32 Width, Core::Int32 Height, float Scale, const std::wstring& ProjectName);

        /**
         * @brief 배경 위에 단계·파일·진행 막대를 그려 대상 DC로 복사합니다.
         * @param[in] Target 프레임을 복사할 창 DC입니다.
         * @param[in] Stage 현재 로딩 단계 이름입니다.
         * @param[in] Action 현재 파일에 수행하는 작업 이름입니다.
         * @param[in] File 처리 중인 파일의 표시용 경로입니다.
         * @param[in] Progress 표시할 진행률이며 0~1 범위입니다.
         * @param[in] Time 진행 막대 반짝임 애니메이션에 쓸 경과 초입니다.
         */
        void Draw(HDC Target, const std::wstring& Stage, const std::wstring& Action, const std::wstring& File, float Progress, float Time);

    private:
        /**
         * @brief 논리 픽셀을 현재 배율의 물리 픽셀로 바꿉니다.
         * @param[in] Logical 96 DPI 기준 길이입니다.
         * @return 반올림한 물리 픽셀 길이입니다.
         */
        Core::Int32 Pixels(float Logical) const;

        /**
         * @brief 실행 파일에 포함된 Pretendard 글꼴을 이 프로세스 전용으로 등록합니다.
         * @return 세 굵기를 모두 등록하면 true입니다.
         */
        bool LoadFonts();

        /**
         * @brief 현재 배율의 Pretendard 글꼴을 만들고, 등록하지 못했으면 Segoe UI로 대신합니다.
         * @param[in] Size 96 DPI 기준 글자 높이입니다.
         * @param[in] Weight FW_NORMAL, FW_SEMIBOLD, FW_BOLD 중 하나인 GDI 글꼴 두께입니다.
         * @return 호출자가 해제해야 하는 글꼴 핸들입니다.
         */
        HFONT MakeFont(float Size, Core::Int32 Weight) const;

        /**
         * @brief 현재 선택된 글꼴로 한 줄 문자열을 그립니다.
         * @param[in] Text 그릴 문자열입니다.
         * @param[in] Bounds 문자열을 배치할 영역입니다.
         * @param[in] Color 0xRRGGBB 형식의 글자색입니다.
         * @param[in] Flags 추가 DrawText 플래그입니다.
         */
        void DrawLine(const std::wstring& Text, RECT Bounds, Core::Uint32 Color, UINT Flags);

        /**
         * @brief 실행 파일의 Macaw 아이콘을 미리 곱한 알파의 RGBA 실수 배열로 읽습니다.
         * @param[in] Size 읽을 아이콘 크기입니다.
         * @return 읽기에 성공하면 true입니다.
         */
        bool LoadMacawIcon(Core::Int32 Size);

        /**
         * @brief 아이콘을 쌍선형 보간으로 표본화합니다.
         * @param[in] U 가로 정규화 좌표입니다.
         * @param[in] V 세로 정규화 좌표입니다.
         * @return 미리 곱한 알파의 RGBA 값입니다.
         */
        std::array<float, 4> SampleIcon(float U, float V) const;

        /**
         * @brief 그라데이션, 빛번짐, 아이콘과 고정 글자를 배경 버퍼에 합성합니다.
         * @param[in] ProjectName 배경에 새길 프로젝트 이름입니다.
         */
        void ComposeBackground(const std::wstring& ProjectName);

        /**
         * @brief 프레임 버퍼에 진행 막대와 반짝임을 그립니다.
         * @param[in] Progress 표시할 진행률이며 0~1 범위입니다.
         * @param[in] Time 반짝임 위치를 정할 경과 초입니다.
         */
        void DrawProgress(float Progress, float Time);

        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 프레임 가로 픽셀 수입니다.
         */
        Core::Int32 mWidth{};
        ReflectProperty(mWidth, "Width", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프레임 세로 픽셀 수입니다.
         */
        Core::Int32 mHeight{};
        ReflectProperty(mHeight, "Height", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 96 DPI 기준 화면 배율입니다.
         */
        float mScale{1.0f};
        ReflectProperty(mScale, "Scale", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프레임 DIB를 선택한 메모리 DC입니다.
         */
        HDC mDC{};
        ReflectProperty(mDC, "DC", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 32비트 프레임 DIB입니다.
         */
        HBITMAP mBitmap{};
        ReflectProperty(mBitmap, "Bitmap", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메모리 DC에 원래 선택되어 있던 비트맵입니다.
         */
        HGDIOBJ mOldBitmap{};
        ReflectProperty(mOldBitmap, "OldBitmap", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프레임 DIB의 0xRRGGBB 픽셀 주소입니다.
         */
        Core::Uint32* mBits{};
        ReflectProperty(mBits, "Bits", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 매 프레임 복사해 쓰는 고정 배경 픽셀입니다.
         */
        std::vector<Core::Uint32> mBackground{};
        ReflectProperty(mBackground, "Background", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 아이콘 한 변의 픽셀 수입니다.
         */
        Core::Int32 mIconSize{};
        ReflectProperty(mIconSize, "IconSize", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 미리 곱한 알파의 RGBA 아이콘 픽셀입니다.
         */
        std::vector<float> mIconPixels{};
        ReflectProperty(mIconPixels, "IconPixels", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief "MACAW" 제목 글꼴입니다.
         */
        HFONT mTitleFont{};
        ReflectProperty(mTitleFont, "TitleFont", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 부제와 라벨 글꼴입니다.
         */
        HFONT mCaptionFont{};
        ReflectProperty(mCaptionFont, "CaptionFont", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프로젝트 이름 글꼴입니다.
         */
        HFONT mProjectFont{};
        ReflectProperty(mProjectFont, "ProjectFont", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 로딩 단계 글꼴입니다.
         */
        HFONT mStageFont{};
        ReflectProperty(mStageFont, "StageFont", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 처리 중 파일 글꼴입니다.
         */
        HFONT mDetailFont{};
        ReflectProperty(mDetailFont, "DetailFont", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief AddFontMemResourceEx로 등록한 Pretendard 글꼴 핸들입니다.
         */
        std::vector<HANDLE> mFontResources{};
        ReflectProperty(mFontResources, "FontResources", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Pretendard 세 굵기를 모두 등록했는지 여부입니다.
         */
        bool mHasPretendard{};
        ReflectProperty(mHasPretendard, "HasPretendard", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
