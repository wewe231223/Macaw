#pragma once 

#include <d3d11.h>
#include "../Core/Channel/FStateChannel.h"
#include "../RenderWindowInfo.h"

class EditorViewport {
public:
	EditorViewport() = default;
	~EditorViewport() = default;

	EditorViewport(const EditorViewport&) = delete;
	EditorViewport& operator=(const EditorViewport&) = delete;

	EditorViewport(EditorViewport&&) noexcept = default;
	EditorViewport& operator=(EditorViewport&&) noexcept = default;

public:
	void Initialize(ID3D11Device* Device, TStateChannel<RenderWindowInfo>::FReader WindowInfoReader);

private:
	TStateChannel<RenderWindowInfo>::FReader WindowInfoReader;

};