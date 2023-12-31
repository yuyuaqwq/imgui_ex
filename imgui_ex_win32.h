#ifndef IMGUI_IMGUI_EX_WIN32_H_
#define IMGUI_IMGUI_EX_WIN32_H_

#include <unordered_map>
#include <string>
#include <exception>
#include <functional>

#ifndef IMGUI_EX_CPP
#include <imgui/imgui.h>
#include <imgui/imconfig.h>
#include <imgui/imgui_internal.h>
#include <imgui/imstb_rectpack.h>
#include <imgui/imstb_textedit.h>

#include <imgui/backends/imgui_impl_dx11.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <d3d11.h>
#endif

void ImGuiInit();
void ImGuiUpdate();
void ImGuiExit();

namespace ImGuiEx {

void ExitApplication();
void SlowDown();

namespace internal {
static HWND GetWindowHwnd(ImGuiWindow* window) {
    if (window == nullptr) return NULL;
    return (HWND)window->Viewport->PlatformHandle;
}
static HWND FindWindowHwndByName(const char* name) {
    auto window = ImGui::FindWindowByName(name);
    return GetWindowHwnd(window);
}
static bool SetWindowTop(ImGuiWindow* window, bool top) {
    //ImGuiWindow* window = ImGui::GetCurrentWindow();
    HWND hwnd = internal::GetWindowHwnd(window);
    if (hwnd == NULL) {
        return false;
    }

    if (top) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    return true;
}
} // namespace internal


class Expandable {
public:
    Expandable() {
        end_expand_ = false;
        expand_ = false;
    }

    void ExpandUpdate(std::function<void()> update) {
        if (expand_ == true) { 
            update(); 
        }
    }

    void CollapsingUpdate(std::function<void()> update) {
        if (expand_ == false) {
            update();
        }
    }

    void ExpandEvent(std::function<void()> event) { \
        if (expand_ == true && expand_ != end_expand_) {
            event();
        }
    }
    
    void CollapsingEvent(std::function<void()> event) {
        if (expand_ == false && expand_ != end_expand_) {
            event();
        }
    }

protected:
    bool end_expand_;
    bool expand_;
};

    

/*
* Event设计原则
* 当前帧对组件的Control，所有会对Event产生影响的场景都要留到下一帧才能触发
*/

class Widget {
public:
    Widget(const std::string& label) : label_(label) {
        entry_disabled_ = false;
        end_disabled_ = false;
        disabled_ = false;
        init_ = false;
    }

    void Begin() {
        if (disabled_) {
            ImGui::BeginDisabled();
            entry_disabled_ = true;
        }
        else {
            entry_disabled_ = false;
        }
    }

    void End() {
        end_disabled_ = disabled_;
        if (entry_disabled_) {
            ImGui::EndDisabled();
        }
    }


    void InitEvent(std::function<void()> init) {
        if (!init_) {
            init_ = true;
            init();
        }
    }

    void DisableEvent(std::function<void()> event) {
        if (disabled_ == true && disabled_ != end_disabled_) {
            event();
        }
    }


    void EnableEvent(std::function<void()> event) {
        if (disabled_ == false && disabled_ != end_disabled_) {
            event();
        }
    }


    std::string GetLabel() {
        return label_;
    }

    void SetLabel(const std::string& label) {
        label_ = label;
    }

    void SetDisable(bool disabled) {
        disabled_ = disabled;
    }

    void Disable() {
        SetDisable(true);
    }

    void Enable() {
        SetDisable(false);
    }

private:
    std::string label_;

    bool init_;

    bool entry_disabled_;
    bool end_disabled_;
    bool disabled_;
};



class Window : public Widget,
               public Expandable
{
public:
    Window(const std::string& label, bool main = false, bool create = true, ImGuiWindowFlags flags = 0) : Widget(label) {
        window_ = nullptr;
        main_ = main;
        end_top_ = true;
        top_ = false;
        control_create_ = false;
        control_close_ = false;
        end_create_ = false;
        create_ = create;

        entry_ = false;

        end_flags_ = flags;
        flags_ = flags;
    }

    void Begin() {
        Widget::Begin();
        
        if (window_ == nullptr) {
            window_ = ImGui::FindWindowByName(GetLabel().c_str());
        }

        if (end_top_ != top_) {
            if (internal::SetWindowTop(window_, top_)) {
                end_top_ = top_;
            }
        }

        if (control_create_) {
            create_ = true;
            control_create_ = false;
        }
        if (control_close_) {
            create_ = false;
            control_close_ = false;
        }

        if (create_) {
            entry_ = true;
            expand_ = ImGui::Begin(GetLabel().c_str(), &create_, flags_);
        }
        if (create_ == true && create_ != end_create_) {
            // 每次重新开启窗口都要重设top状态
            end_top_ = !top_;
        }
    }

    void End() {
        if (entry_) {
            ImGui::End();
            entry_ = false;
        }
        end_create_ = create_;
        if (create_ == false) {
            if (main_) {
                ExitApplication();
            }
        }
        end_expand_ = expand_;
        Widget::End();
    }

    /*
    * Update
    */
    void CreateUpdate(std::function<void()> event) {
        if (create_) {
            event();
        }
    }

    void CloseUpdate(std::function<void()> close) {
        if (!create_) {
            close();
        }
    }


    /*
    * Event
    */

    void CreateEvent(std::function<void()> event) {
        if (end_create_ == false && create_ == true) {
            event();
        }
    }

    void CloseEvent(std::function<void()> event) {
        if (end_create_ == true && create_ == false) {
            event();
        }
    }


    /*
    * Control
    */
    void Create() {
        control_create_ = true;
    }

    void Close() {
        control_close_ = true;
    }


    ImGuiWindowFlags GetFlags() {
        return flags_;
    }

    void SetFlags(ImGuiWindowFlags flags) {
        flags_ = flags;
    }

    void AddFlags(ImGuiWindowFlags flags) {
        flags_ |= flags;
    }

    void UnaddFlags(ImGuiWindowFlags flags) {
        flags_ &= (~flags);;
    }


    void SetTop(bool top) {
        top_ = top;
    }

    void SetDocking(bool enable){
        if (enable) {
            UnaddFlags(ImGuiWindowFlags_NoDocking);
        }
        else {
            AddFlags(ImGuiWindowFlags_NoDocking);
        }
    }

    void SetAlwaysAutoResize(bool enable) {
        if (enable) {
            AddFlags(ImGuiWindowFlags_AlwaysAutoResize);
        }
        else {
            UnaddFlags(ImGuiWindowFlags_AlwaysAutoResize);
        }
    }

    void SetMove(bool enable) {
        if (enable) {
            UnaddFlags(ImGuiWindowFlags_NoMove);
        }
        else {
            AddFlags(ImGuiWindowFlags_NoMove);
        }
    }

    void SetCollapse(bool enable) {
        if (enable) {
            UnaddFlags(ImGuiWindowFlags_NoCollapse);
        }
        else {
            AddFlags(ImGuiWindowFlags_NoCollapse);
        }
    }

private:
    ImGuiWindow* window_;

    bool main_;
    bool entry_;
    bool end_top_;
    bool top_;

    bool control_close_;
    bool control_create_;
    bool end_create_;
    bool create_;

    ImGuiWindowFlags end_flags_;
    ImGuiWindowFlags flags_;
};

class Button : public Widget {
public:
    Button(const std::string& label) : Widget(label) {
        click_ = false;
        control_click_ = false;
    }

    void Begin() {
        Widget::Begin();
        click_ = ImGui::Button(GetLabel().c_str());
        if (control_click_) {
            click_ = true;
            control_click_ = false;
        }
    }

    void End() {
        Widget::End();
    }

    /*
    * Event
    */
    void ClickEvent(std::function<void()> event) {
        if (click_ == true) {
            event();
        }
    }


    /*
    * Control
    */
    void Click() {
        control_click_ = true;
    }

private:
    bool control_click_;
    bool click_;
};

template<class Element = std::string>
class Combo : public Widget,
              public Expandable
{
public:
    Combo(const std::string& label) : Widget(label) {
        end_select_index_ = -1;
        select_index_ = -1;

        end_expand_ = false;
        expand_ = false;
    }

    void Begin() {
        Widget::Begin();
        expand_ = ImGui::BeginCombo(GetLabel().c_str(), select_label_.c_str());
    }

    void End() {
        if (expand_) {
            ImGui::EndCombo();
        }
        end_expand_ = expand_;
        end_select_index_ = select_index_;
        Widget::End();
    }

    /*
    * Update
    */
    void InsertUpdate(std::function<std::string(Element&)> insert) {
        if (expand_ == false) {
            return;
        }
        for (int i = 0; i < list_.size(); i++) {
            const bool is_selected = (select_index_ == i);
            auto temp = insert(list_[i]);

            if (temp.empty()) {
                continue;
            }

            if (ImGui::Selectable(temp.c_str(), is_selected)) {
                select_index_ = i;
                select_label_ = temp;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        
    }

    /*
    * Event
    */
    void SelectEvent(std::function<void()> event) {
        if (end_select_index_ != select_index_) {
            event();
        }
    }


    /*
    * Control
    */
    Element& GetSelectItem() {
        return list_.at(select_index_);
    }

    int GetSelectIndex() {
        return select_index_;
    }

    int SetSelectIndex(int select_index) {
        select_index_ = select_index;
    }

    void SetList(std::vector<Element>&& list) {
        list_ = std::move(list);
    }

    std::vector<Element>& GetList() {
        return list_;
    }

    void ClearList() {
        list_.clear();
    }

private:
    std::vector<Element> list_;
    int end_select_index_;
    int select_index_;
    std::string select_label_;

};

class InputText : public Widget {
public:
    InputText(const std::string& label, size_t text_size) : Widget(label), text_(text_size, '\0') {
        end_input_ = false;
        input_ = false;
    }

    void Begin() {
        Widget::Begin();
        if (ImGui::InputText(GetLabel().c_str(), (char*)text_.c_str(), text_.size())) {
            input_ = true;
        } else {
            input_ = false;
        }
    }

    void End() {
        end_input_ = input_;
        Widget::End();
    }

    /*
    * Event
    */
    void InputEvent(std::function<void()> event) {
        if (input_ == true && input_ != end_input_) {
            event();
        }
    }

    std::string GetText() {
        return std::string(text_.c_str(), strlen(text_.c_str()));
    }

    void SetText(const std::string& text) {
        memcpy((void*)text_.c_str(), text.c_str(), (text_.size() < text.size() ? text_.size() : text.size()));
    }

private:
    std::string text_;

    bool end_input_;
    bool input_;
};

class InputTextMultiline : public Widget {
public:
    InputTextMultiline(const std::string& label, const std::string& text = "", ImGuiInputTextFlags flags = 0) : Widget(label), size_(-FLT_MIN, -FLT_MIN) {
        end_input_ = false;
        input_ = false;

        flags_ = flags | ImGuiInputTextFlags_CallbackResize;
        SetText(text);
    }

    void Begin() {
        if (text_.empty()) {
            text_.push_back('\0');
        }
        Widget::Begin();
        if (ImGui::InputTextMultiline(GetLabel().c_str(), text_.begin(), text_.size(), size_, flags_, ResizeCallback, &text_)) {
            input_ = true;
        }
        else {
            input_ = false;
        }
    }

    void End() {
        end_input_ = input_;
        Widget::End();
    }

    /*
    * Event
    */
    void InputEvent(std::function<void()> event) {
        if (input_ == true && input_ != end_input_) {
            event();
        }
    }

    std::string GetText() {
        return std::string(&text_[0], strlen(&text_[0]));
    }

    void SetText(const std::string& text) {
        text_.clear();
        text_.resize(text.size() + 1);
        memcpy(&text_[0], &text[0], text.size() + 1);
    }

    void SetSize(const ImVec2& size) {
        size_ = size;
    }

    ImVec2& GetSize() {
        return size_;
    }

    void SetReadOnly(bool enable) {
        if (enable) {
            flags_ |= ImGuiInputTextFlags_ReadOnly;
        }
        else {
            flags_ &= (~ImGuiInputTextFlags_ReadOnly);
        }
    }

private:
    static int ResizeCallback(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            ImVector<char>* my_str = (ImVector<char>*)data->UserData;
            IM_ASSERT(my_str->begin() == data->Buf);
            my_str->resize(data->BufSize); // NB: On resizing calls, generally data->BufSize == data->BufTextLen + 1
            data->Buf = my_str->begin();
        }
        return 0;
    }

private:
    ImVector<char> text_;

    bool end_input_;
    bool input_;

    ImVec2 size_;

    ImGuiInputTextFlags flags_;
};

class Text : public Widget {
public:
    template<typename ... Args>
    Text(const char* fmt, Args... args) : Widget(fmt) {
        
    }

    void Begin() {
        Widget::Begin();
        ImGui::Text(GetLabel().c_str());
    }

    void End() {
        Widget::End();
    }


    std::string GetText() {
        return GetLabel();
    }

    void SetText(const std::string& text) {
        return SetLabel(text);
    }

private:
    std::string text_;
};

class SeparatorText : public Widget {
public:
    SeparatorText(const std::string& label) : Widget(label) {
        
    }

    void Begin() {
        Widget::Begin();
        ImGui::SeparatorText(GetLabel().c_str());
    }

    void End() {
        Widget::End();
    }
};

class BulletText : public Widget {
public:
    template<typename ... Args>
    BulletText(const char* fmt, Args... args) : Widget(fmt) {
        ImGui:;BulletText(fmt, args...);
    }

    void Begin() {
        Widget::Begin();
        ImGui::BulletText(GetLabel().c_str());
    }

    void End() {
        Widget::End();
    }
};

class HelpMarker : public Widget {
public:
    HelpMarker(const std::string& title, const std::string& desc) : Widget(title), desc_(desc) {
        
    }

    void Begin() {
        Widget::Begin();
        
        ImGui::TextDisabled(GetLabel().c_str());
        if (ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc_.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void End() {
        Widget::End();
    }

private:
    std::string desc_;
};


class CollapsingHeader : public Widget,
                         public Expandable
{
public:
    CollapsingHeader(const std::string& label) : Widget(label) {
        
    }

    void Begin() {
        Widget::Begin();
        expand_ = ImGui::CollapsingHeader(GetLabel().c_str());
    }

    void End() {
        end_expand_ = expand_;
        Widget::End();
    }

private:

};

class TreeNode : public Widget,
                 public Expandable
{
public:
    TreeNode(const std::string& label) : Widget(label) {
        end_expand_ = false;
        expand_ = false;
    }

    void Begin() {
        Widget::Begin();
        expand_ = ImGui::TreeNode(GetLabel().c_str());
    }

    void End() {
        if (expand_) {
            ImGui::TreePop();
        }
        end_expand_ = expand_;
        Widget::End();
    }


    /*
    * Event
    */
private:
};

class CheckBox : public Widget {
public:
    CheckBox(const std::string& label, bool check = false) : Widget(label) {
        end_check_ = false;
        check_ = check;
    }

    void Begin() {
        Widget::Begin();
        ImGui::Checkbox(GetLabel().c_str(), &check_);
    }

    void End() {
        end_check_ = check_;
        Widget::End();
    }


    /*
    * Event
    */
    void CheckEvent(std::function<void()> event) {
        if (end_check_ == false && check_ == true) {
            event();
        }
    }

    void UncheckEvent(std::function<void()> event) {
        if (end_check_ == true && check_ == false) {
            event();
        }
    }

    /*
    * Control
    */
    void SetCheck(bool check) {
        check_ = check;
    }

private:
    bool end_check_;
    bool check_;
};

template<class Element = std::string>
class ListBox : public Widget {
public:
    ListBox(const std::string& label) : Widget(label), size_(-FLT_MIN, -FLT_MIN) {
        entry_ = false;
        end_select_index_ = -1;
        select_index_ = -1;
    }

    void Begin() {
        Widget::Begin();
        entry_ = ImGui::BeginListBox(GetLabel().c_str(), size_);
    }

    void End() {
        if (entry_) {
            ImGui::EndListBox();
        }
        Widget::End();
    }

    /*
    * Update
    */
    void InsertUpdate(std::function<std::string(Element&)> insert) {
        for (int i = 0; i < list_.size(); i++) {
            const bool is_selected = (select_index_ == i);
            auto temp = insert(list_[i]);

            if (temp.empty()) {
                continue;
            }

            if (ImGui::Selectable(temp.c_str(), is_selected)) {
                select_index_ = i;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
    }

    /*
    * Event
    */
    void SelectEvent(std::function<void()> event) {
        if (end_select_index_ != select_index_) {
            event();
        }
    }

    /*
    * Control
    */
    Element& GetSelectItem() {
        return list_.at(select_index_);
    }

    int GetSelectIndex() {
        return select_index_;
    }

    int SetSelectIndex(int select_index) {
        select_index_ = select_index;
    }

    void SetSize(const ImVec2& size) {
        size_ = size;
    }

    ImVec2& GetSize() {
        return size_;
    }

    void SetList(std::vector<Element>&& list) {
        list_ = std::move(list);
    }

    std::vector<Element>& GetList() {
        return list_;
    }

    void ClearList() {
        list_.clear();
    }

private:
    std::vector<Element> list_;

    ImVec2 size_;

    bool entry_;

    int end_select_index_;
    int select_index_;
};

class RadioButtonGroup : public Widget {
public:
    RadioButtonGroup(std::vector<std::string> label_list) : Widget(""), label_list_(label_list){
        push_index_ = 0;
        select_index_ = 0;
        end_select_index_ = 0;
    }

    void Begin(std::function<void(size_t)> push) {
        Widget::Begin();
        push_index_ = 0;
        for (size_t i = 0; i < label_list_.size(); i++) {
            ImGui::RadioButton(label_list_[i].c_str(), &select_index_, push_index_++);
            push(i);
        }
    }

    void Begin() {
        Begin([](size_t i) {});
    }

    void End() {
        end_select_index_ = select_index_;
        Widget::End();
    }


    void SelectEvent(std::function<void()> event) {
        if (end_select_index_ != select_index_) {
            event();
        }
    }

private:
    std::vector<std::string> label_list_;

    int push_index_;
    int end_select_index_;
    int select_index_;
};


namespace layout {
    static void Indent() {
        ImGui::Indent();
    }

    static void Unindent() {
        ImGui::Unindent();
    }

    static void Spacing() {
        ImGui::Spacing();
    }

    static void SameLine() {
        ImGui::SameLine();
    }
}

} // namespace ImGuiEx


#endif // IMGUI_IMGUI_EX_WIN32_H_
