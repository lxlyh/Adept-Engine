#include "UIListBox.h"
#include "../BasicWidgets/UIBox.h"
#include "../BasicWidgets/UILabel.h"
#include "../Core/UIWidget.h"
#include "../Core/Layout.h"
UIListBox::UIListBox(int w, int h, int x, int y) : UIWidget(w, h, x, y)
{

	Background = new UIBox(w, h, x, y);
	Background->Colour = glm::vec3(0.7f);
	TitleLabel = new UILabel("List Box", w, 20, x, y + mheight - 20);
	AddChild(Background);
	AddChild(TitleLabel);
	BatchMode = EWidgetBatchMode::On;
}


UIListBox::~UIListBox()
{}

void UIListBox::Render()
{
	Background->Render();
	TitleLabel->Render();
	for (int i = 0; i < items.size(); i++)
	{
		items[i]->Render();
	}
}
void UIListBox::MouseMove(int x, int y)
{
	for (int i = 0; i < items.size(); i++)
	{
		items[i]->MouseMove(x, y);
		Background->MouseMove(x, y);
	}
}

bool UIListBox::MouseClick(int x, int y)
{
	bool ReturnValue = false;
	for (int i = 0; i < items.size(); i++)
	{
		ReturnValue |= items[i]->MouseClick(x, y);
		ReturnValue |= Background->MouseClick(x, y);
	}
	return ReturnValue;
}
void UIListBox::MouseClickUp(int x, int y)
{
	for (int i = 0; i < items.size(); i++)
	{
		items[i]->MouseClickUp(x, y);
		Background->MouseClickUp(x, y);
	}
}
void UIListBox::ResizeView(int w, int h, int x, int y)
{
	UIWidget::ResizeView(w, h, x, y);
	Background->ResizeView(w, h, X, Y);

	TitleLabel->ResizeView(w, 10, X, Y + mheight - 20);
	UIUtils::ArrangeHorizontal(w - Edgespace, h - 40, x + Edgespace / 2, y, items, 0.05f, 0, 20);
	//UIUtils::ArrangeGrid(w, h - 40, x, y, items, 5);
}
void UIListBox::UpdateScaled()
{
	UIWidget::UpdateScaled();
	for (int i = 0; i < items.size(); i++)
	{
		items[i]->UpdateScaled();
	}
}
void UIListBox::RemoveAll()
{
	for (int i = 0; i < items.size(); i++)
	{
		delete items[i];
	}
	CurrentCount = 0;
	items.clear();
}
void UIListBox::Select(int index)
{
	SelectedCurrent = index;
	if (SelectionChanged)
	{
		SelectionChanged(SelectedCurrent);
	}
	for (int i = 0; i < items.size(); i++)
	{
		items[i]->SetSelected(i == SelectedCurrent);
	}
}
void UIListBox::AddItem(std::string name)
{
	UIButton* button = new UIButton(mwidth - Edgespace, ItemHeight, Edgespace / 2, (Y + mheight) - (TitleHeight + (CurrentCount + 1) * Spacing));
	//button->GetLabel()->TextScale = 0.3f;
	button->SetText(name);
	button->AligmentStruct.SizeMax = 0.03f;
	button->BindTarget(std::bind(&UIListBox::Select, this, CurrentCount));
	button->BackgoundColour = Background->Colour;
	items.push_back(button);
	CurrentCount++;
	AddChild(button);
}

void UIListBox::SetTitle(std::string name)
{
	TitleLabel->SetText(name);
}
