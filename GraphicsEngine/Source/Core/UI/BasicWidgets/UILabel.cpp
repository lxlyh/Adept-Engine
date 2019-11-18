#include "UILabel.h"
#include "UI/UIManager.h"
#include "Rendering/Renderers/TextRenderer.h"
#include "../Core/UIWidgetContext.h"
#include "../Core/UIDrawBatcher.h"

UILabel::UILabel(std::string  text, int w, int h, int x, int y) : UIWidget(w, h, x, y)
{
	MText = text;
	TextScale = TextDefaultScale;
	Colour = glm::vec3(1);
}


UILabel::~UILabel()
{}

void UILabel::Render()
{}

void UILabel::ResizeView(int w, int h, int x, int y)
{
	mheight = h;
	mwidth = w;
	X = x;
	Y = y;
}

void UILabel::SetText(const std::string& text)
{
	MText = text;
}

std::string UILabel::GetText()
{
	return MText;
}

void UILabel::OnGatherBatches(UIRenderBatch* Groupbatchptr /*= nullptr*/)
{
	if (MText.length() == 0)
	{
		return;
	}
	UIRenderBatch* batch = Groupbatchptr;
	if (batch == nullptr)
	{
		batch = new UIRenderBatch();
	}
	batch->AddText(MText, glm::vec2(GetOwningContext()->Offset.x + (float)X + 10, GetOwningContext()->Offset.y + (float)Y + ((mheight / 2.0f) - (TextScale))));
	if (Groupbatchptr == nullptr)
	{
		GetOwningContext()->GetBatcher()->AddBatch(batch);
	}
}