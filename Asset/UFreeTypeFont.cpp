#include "pch.h"
#include "UFreeTypeFont.h"

UFreeTypeFont::~UFreeTypeFont() {
    Reset();
}

void UFreeTypeFont::Reset() {
    // SRV가 AtlasTexture를 참조하므로 View를 먼저 해제하고 Texture를 해제한다.
    mAtlasSrv.Reset();
    mAtlasTexture.Reset();

    if (mFace != nullptr) {
        FT_Done_Face(mFace);
        mFace = nullptr;
    }

    if (mLibrary != nullptr) {
        FT_Done_FreeType(mLibrary);
        mLibrary = nullptr;
    }

    mCodePointToGlyphIndex.clear();
    mGlyphCache.clear();

    mAtlasPixels.clear();

    mFontMetrics = {};

    mAtlasWidth = 0;
    mAtlasHeight = 0;

    mNextAtlasX = 0;
    mNextAtlasY = 0;
    mCurrentRowHeight = 0;

    mBAtlasDirty = false;
    mBInitialized = false;
}

bool UFreeTypeFont::Initialize(ID3D11Device* Device, const std::filesystem::path& FontPath, Uint32 BakePixelHeight, Uint32 AtlasWidth, Uint32 AtlasHeight) {
    if (!UAsset::Initialize(Device, FontPath)) {
        return false;
    }

    return InitializeFont(Device, FontPath, BakePixelHeight, AtlasWidth, AtlasHeight);
}

bool UFreeTypeFont::InitializeFont(ID3D11Device* Device, const std::filesystem::path& FontPath, Uint32 BakePixelHeight, Uint32 InAtlasWidth, Uint32 InAtlasHeight) {
    Reset();

    if (Device == nullptr || FontPath.empty() || BakePixelHeight == 0 || InAtlasWidth == 0 || InAtlasHeight == 0) {
        return false;
    }

    mAtlasWidth = InAtlasWidth;
    mAtlasHeight = InAtlasHeight;

    FT_Error Error{FT_Init_FreeType(&mLibrary)}; // FreeType 라이브러리 시작
    if (Error != FT_Err_Ok) {
        Reset();
        return false;
    }
    Error = FT_New_Face(mLibrary, FontPath.string().c_str(), 0, &mFace); // TTF 파일에서 mFace 생성
    if (Error != FT_Err_Ok) {
        Reset();
        return false;
    }
    Error = FT_Select_Charmap(mFace, FT_ENCODING_UNICODE); // Unicode CodePoint를 사용할 charmap 선택
    if (Error != FT_Err_Ok) {
        Reset();
        return false;
    }
    Error = FT_Set_Pixel_Sizes(mFace, 0, BakePixelHeight); // Glyph를 생성할 픽셀 크기 설정
    // 폰트 전체 공통 Metric 저장
    mFontMetrics.mBakePixelHeight = static_cast<float>(BakePixelHeight);
    mFontMetrics.mAscender = static_cast<float>(mFace->size->metrics.ascender) / 64.0f;
    mFontMetrics.mDescender = static_cast<float>(mFace->size->metrics.descender) / 64.0f;
    mFontMetrics.mLineHeight = static_cast<float>(mFace->size->metrics.height) / 64.0f;
    // 빈 CPU Atlas 생성
    const std::size_t AtlasPixelCount{static_cast<std::size_t>(mAtlasWidth) * static_cast<std::size_t>(mAtlasHeight)};
    mAtlasPixels.assign(AtlasPixelCount, std::uint8_t{0});
    if (!CreateAtlasTexture(Device)) {
        Reset();
        return false;
    }
    mBAtlasDirty = false;
    mBInitialized = true;
    return true;
}

bool UFreeTypeFont::CreateAtlasTexture(ID3D11Device* Device) {
    if (Device == nullptr || mAtlasPixels.empty()) {
        return false;
    }

    mAtlasSrv.Reset();
    mAtlasTexture.Reset();

    D3D11_TEXTURE2D_DESC TextureDesc{};
    TextureDesc.Width = mAtlasWidth;
    TextureDesc.Height = mAtlasHeight;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA InitialData{};
    InitialData.pSysMem = mAtlasPixels.data();
    InitialData.SysMemPitch = mAtlasWidth * sizeof(std::uint8_t);
    InitialData.SysMemSlicePitch = 0;

    HRESULT Result{Device->CreateTexture2D(&TextureDesc, &InitialData, mAtlasTexture.GetAddressOf())};

    if (FAILED(Result)) {
        return false;
    }

    Result = Device->CreateShaderResourceView(mAtlasTexture.Get(), nullptr, mAtlasSrv.GetAddressOf());

    if (FAILED(Result)) {
        mAtlasTexture.Reset();
        return false;
    }

    return true;
}

void UFreeTypeFont::FlushAtlas(ID3D11DeviceContext* Context) {
    if (!mBInitialized || !mBAtlasDirty || Context == nullptr || mAtlasTexture == nullptr || mAtlasPixels.empty()) {
        return;
    }

    Context->UpdateSubresource(
        mAtlasTexture.Get(),                // 갱신할 GPU Texture
        0,                                  // 첫번째 mip level
        nullptr,                            // Texture 전체 갱신
        mAtlasPixels.data(),                // CPU 픽셀 시작 주소
        mAtlasWidth * sizeof(std::uint8_t), // CPU 한 행의 바이트 수
        0);                                 // 2D Texture 이므로 사용 X

    mBAtlasDirty = false;
}

const FFontGlyph* UFreeTypeFont::FindGlyph(char32_t CodePoint) const {
    auto CodePointIt{mCodePointToGlyphIndex.find(CodePoint)};
    if (CodePointIt == mCodePointToGlyphIndex.end()) {
        return nullptr;
    }
    auto GlyphCacheIt{mGlyphCache.find(CodePointIt->second)};
    if (GlyphCacheIt == mGlyphCache.end()) {
        return nullptr;
    }
    return &GlyphCacheIt->second;
}

const FFontGlyph* UFreeTypeFont::GetOrCreateGlyph(char32_t CodePoint) {
    if (!mBInitialized || mFace == nullptr) {
        return nullptr;
    }
    // FindGlyph 결과 Glyph가 존재한다면 해당 Glyph 반환
    if (const FFontGlyph* CachedGlyph{FindGlyph(CodePoint)}) {
        return CachedGlyph;
    }
    // 없다면 우선 첫번째로 GlyphIndex 매핑은 됐으나 GlyphCache에 등록되지 않은 경우를 찾아본다.
    // 즉, 다른 CodePoint로 같은 GlyphIndex가 나온 것이다. FT_Face에서 같은 모양을 가리킨다고 판단한것이다.
    std::uint32_t GlyphIndex{0};
    const auto CodePointIt{mCodePointToGlyphIndex.find(CodePoint)};
    if (CodePointIt != mCodePointToGlyphIndex.end()) {
        // 해당 CodePoint와 GlyphIndex를 매핑한다.
        GlyphIndex = CodePointIt->second;
    } else {
        // 두번째원인은, 처음으로 캐싱하는 Glyph인 경우이다. 해당 폰트에서 GlyphIndex를 받는다.
        GlyphIndex = static_cast<std::uint32_t>(FT_Get_Char_Index(mFace, static_cast<FT_ULong>(CodePoint)));
        // GlyphIndex가 0인 경우, 해당 글씨가 폰트에 존재하지 않는것이다.
        if (GlyphIndex == 0) {
            return nullptr;
        }
    }
    // 다른 CodePoint로 같은 GlyphIndex가 나온경우 해당 CodePoint에 매핑되는 Glyph가 존재하므로
    const auto GlyphCacheIt{mGlyphCache.find(GlyphIndex)};
    if (GlyphCacheIt != mGlyphCache.end()) {
        // 해당 관계(CodePoint->GlyphIndex)를 등록하고
        mCodePointToGlyphIndex.insert_or_assign(CodePoint, GlyphIndex);
        // 캐시의 Glyph를 사용한다.
        return &GlyphCacheIt->second;
    }
    // 캐시에 glyph가 없다면 Create 한다.
    // Glyph outline과 metric 로드
    // TTF 폰트는 글자의 윤곽선을 점과 곡선으로 저장한다. -> outline -> glyph를 구성하는 정보가 들어있다.
    // Metric은 glyph를 어디에 놓고 다음 glyph로 얼마나 이동할지를 알려주는 숫자이다.
    FT_Error Error{FT_Load_Glyph(mFace, static_cast<FT_UInt>(GlyphIndex), FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP)};
    if (Error != FT_Err_Ok) {
        return nullptr;
    }
    // Glyph를 outline에서 Bitmap으로 전환
    Error = FT_Render_Glyph(mFace->glyph, FT_RENDER_MODE_NORMAL);
    // FT_Err_Ok -> 성공인 경우 0
    if (Error != FT_Err_Ok) {
        return nullptr;
    }

    FT_GlyphSlot Slot{mFace->glyph};
    FT_Bitmap& Bitmap{Slot->bitmap};

    // FKGlyph 구조체에 배치 정보 복사
    FFontGlyph NewGlyph{};
    NewGlyph.mGlyphIndex = GlyphIndex;
    NewGlyph.mBitmapWidth = static_cast<std::uint32_t>(Bitmap.width);
    NewGlyph.mBitmapHeight = static_cast<std::uint32_t>(Bitmap.rows);
    NewGlyph.mBearingX = static_cast<std::int32_t>(Slot->bitmap_left);
    NewGlyph.mBearingY = static_cast<std::int32_t>(Slot->bitmap_top);
    // FreeType의 Advance는 26.6(정수 26bit / 소숫점 6bit) 고정소수점을 사용하므로 실제값을 얻으려면 64로 나눠야함.
    // FreeType은 소숫점이 포함된 픽셀 정밀도를 유지하기 위해 이를 사용한다.
    NewGlyph.mAdvanceX = static_cast<float>(Slot->advance.x) / 64.0f;
    NewGlyph.mAdvanceY = static_cast<float>(Slot->advance.y) / 64.0f;

    // Atlas 공간 할당
    if (Bitmap.width > 0 && Bitmap.rows > 0) {
        std::uint32_t AtlasX{0};
        std::uint32_t AtlasY{0};
        // AtlasX, Y를 업데이트하고 Bitmap을 AtlasPixels에 복사한다.
        if (!AllocateAtlasRect(static_cast<std::uint32_t>(Bitmap.width), static_cast<std::uint32_t>(Bitmap.rows), AtlasX, AtlasY)) {
            return nullptr;
        }
        if (!CopyBitmapToAtlas(Bitmap, AtlasX, AtlasY)) {
            return nullptr;
        }
        // 확정된 AtlasX, Y를 바탕으로 나머지 정보들을 업데이트한다.
        NewGlyph.mAtlasX = AtlasX;
        NewGlyph.mAtlasY = AtlasY;
        NewGlyph.mUvMin = FVector2{static_cast<float>(AtlasX) / static_cast<float>(mAtlasWidth), static_cast<float>(AtlasY) / static_cast<float>(mAtlasHeight)};
        NewGlyph.mUvMax = FVector2{static_cast<float>(AtlasX + Bitmap.width) / static_cast<float>(mAtlasWidth), static_cast<float>(AtlasY + Bitmap.rows) / static_cast<float>(mAtlasHeight)};
        mBAtlasDirty = true;
    }
    // 캐시에 저장
    // emplace는 (Iterator, bool) 로 반환함, move -> Glyph를 복사하기보단 이동
    auto [InsertedIt, bInserted]{mGlyphCache.emplace(GlyphIndex, std::move(NewGlyph))};
    if (!bInserted) {
        return nullptr;
    }
    // CodePoint 매핑에도 저장
    mCodePointToGlyphIndex.insert_or_assign(CodePoint, GlyphIndex);
    // 캐시 내부의 glyph 주소 반환
    return &InsertedIt->second;
}

const FFontMetrics& UFreeTypeFont::GetFontMetrics() const {
    return mFontMetrics;
}

// 현재 NextAtlasX,Y에서 해당 BitmapWidth/Height 를 할당할 수 있는지 확인하고 비트맵에 배치될 좌상단 좌표를 OutAtlasX,Y에 업데이트.
// 이후 NextAtlasX,Y를 업데이트
bool UFreeTypeFont::AllocateAtlasRect(std::uint32_t BitmapWidth, std::uint32_t BitmapHeight, std::uint32_t& OutAtlasX, std::uint32_t& OutAtlasY) {
    if (BitmapWidth <= 0 || BitmapHeight <= 0) {
        return false;
    }
    std::uint32_t ReqWidth{BitmapWidth + AtlasPadding * 2};
    std::uint32_t ReqHeight{BitmapHeight + AtlasPadding * 2};
    std::uint32_t TempX{mNextAtlasX};
    std::uint32_t TempY{mNextAtlasY};
    if (TempX + ReqWidth > mAtlasWidth) {
        TempX = 0;
        TempY += mCurrentRowHeight;
        mCurrentRowHeight = ReqHeight;
    }
    if (TempY > mAtlasHeight || TempY + ReqHeight > mAtlasHeight || TempY + mCurrentRowHeight > mAtlasHeight) {
        return false;
    }
    OutAtlasX = TempX + AtlasPadding;
    OutAtlasY = TempY + AtlasPadding;
    mCurrentRowHeight = std::max(mCurrentRowHeight, ReqHeight);
    mNextAtlasX = TempX + ReqWidth;
    mNextAtlasY = TempY;
    return true;
}

bool UFreeTypeFont::CopyBitmapToAtlas(FT_Bitmap& Bitmap, std::uint32_t AtlasX, std::uint32_t AtlasY) {
    if (Bitmap.width == 0 || Bitmap.rows == 0) {
        // 공백의 bitmap
        return true;
    }
    if (Bitmap.buffer == nullptr) {
        return false;
    }
    if (Bitmap.width + AtlasX > mAtlasWidth || Bitmap.rows + AtlasY > mAtlasHeight) {
        return false;
    }
    // Pitch = 다음 행까지의 거리 | 실제 픽셀의 길이를 나타내는 width와 다르다.
    // width는 bitmap에서 최대 가로 길이 이기도 하므로 Pitch는 Padding에 대비함이다.
    const std::uint32_t SourcePitch{static_cast<std::uint32_t>(Bitmap.pitch >= 0 ? Bitmap.pitch : -Bitmap.pitch)};
    if (SourcePitch < Bitmap.width) {
        return false;
    }
    for (std::uint32_t Row{0}; Row < Bitmap.rows; Row++) {
        const std::uint8_t* SourceRow{nullptr};
        /* 일반적인 경우 */
        // buffer의 첫번째 행은 bitmap의 첫번째 행
        if (Bitmap.pitch >= 0) {
            SourceRow = Bitmap.buffer + static_cast<std::size_t>(Row) * SourcePitch;
        }
        /* 방향이 반대인 경우 */
        // buffer의 첫번째 행은 bitmap의 아래쪽 행
        else {
            SourceRow = Bitmap.buffer + static_cast<std::size_t>(Bitmap.rows - 1 - Row) * SourcePitch;
        }
        // AtlasPixels[0] + (Atlas시작 Y픽셀 위치 + 탐색중인 Bitmap row) * Atlas너비 + Atlas시작 X픽셀 위치 = AtlasPixels 기록 시작 위치
        std::uint8_t* DestinationRow{mAtlasPixels.data() + static_cast<std::size_t>(AtlasY + Row) * mAtlasWidth + AtlasX};
        // SourceRow 부터 Bitmap.width 만큼의 정보를 DestinationRow에 복사
        std::memcpy(DestinationRow, SourceRow, Bitmap.width);
    }
    // Atlas 정보 업데이트됨
    mBAtlasDirty = true;
    return true;
}

ID3D11ShaderResourceView* UFreeTypeFont::GetAtlasSRV() const {
    return mAtlasSrv.Get();
}
