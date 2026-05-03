#pragma once
#include <map>
#include "DataModels.h"
#include "UIHelper.h"

class RestaurantApp {
private:
    // ── Window / rendering ────────────────────────────────────────────────
    RenderWindow window;
    Font         font;
    Clock        clock;
    View         gameView;

    // ── Session state ─────────────────────────────────────────────────────
    Screen   currentScreen;
    UserRole currentRole;
    bool     loggedIn;
    Staff* currentStaff;
    Customer* currentCustomer;

    // ── Status bar ────────────────────────────────────────────────────────
    string statusMessage;
    Clock  statusClock;

    // ── Data stores ───────────────────────────────────────────────────────
    vector<MenuItem>       menu;
    vector<Customer>       customers;
    vector<Order>          orders;
    vector<Staff>          staff;
    vector<Table>          tables;
    vector<FeedbackRecord> feedbacks;
    vector<WaitingEntry>   waitQueue;

    int nextMenuId;
    int nextOrderId;
    int nextCustomerId;

    // ── Login screen state ────────────────────────────────────────────────
    string loginUsernameInput;
    string loginPasswordInput;
    bool   typingUsername;
    bool   typingPassword;
    bool   loginAsCustomer;

    // ── Registration form state ───────────────────────────────────────────
    string regNameInput, regPhoneInput, regUserInput, regPassInput;
    bool   typingRegName, typingRegPhone, typingRegUser, typingRegPass;
    bool   showRegisterForm;

    // ── Menu screen state ─────────────────────────────────────────────────
    int    menuScrollOffset;
    string menuNameInput, menuPriceInput, menuCatInput;
    bool   typingMenuName, typingMenuPrice, typingMenuCat;
    int    selectedMenuItemForEdit;
    string menuEditPriceInput;
    bool   typingMenuEditPrice;

    // ── Order screen state ────────────────────────────────────────────────
    int    selectedTableForOrder;
    int    orderMenuScroll;
    string orderCustomerInput;
    bool   typingOrderCustomer;
    Order* activeOrder;
    string orderQtyStr;
    bool   typingOrderQty;
    int    selectedMenuItemIndex;

    // ── Billing screen state ──────────────────────────────────────────────
    int selectedOrderForBill;

    // ── Reservation screen state ──────────────────────────────────────────
    string reserveNameInput, reserveTimeInput;
    int    selectedTableForRes;
    bool   typingResName, typingResTime;

    // ── Customer screen state ─────────────────────────────────────────────
    string custNameInput, custPhoneInput;
    bool   typingCustName, typingCustPhone;
    int    custScrollOffset;

    // ── Kitchen screen state ──────────────────────────────────────────────
    int kitchenScrollOffset;

    // ── Feedback screen state ─────────────────────────────────────────────
    string feedbackCustInput, feedbackRatingInput, feedbackCommentInput;
    bool   typingFbCust, typingFbRating, typingFbComment;

    // ── Queue screen state ────────────────────────────────────────────────
    string queueNameInput, queueSizeInput, queueTimeInput;
    bool   typingQueueName, typingQueueSize, typingQueueTime;

    // ═════════════════════════════════════════════════════════════════════
    //  PRIVATE HELPERS
    // ═════════════════════════════════════════════════════════════════════

    bool clicked(Event& e, float x, float y, float w, float h) {
        return UIHelper::isClicked(e, window, x, y, w, h);
    }

    void showStatus(const string& msg) {
        statusMessage = msg;
        statusClock.restart();
    }

    void handleTextInput(Event& event, string& str, bool isActive) {
        if (!isActive || event.type != Event::TextEntered) return;
        if (event.text.unicode == 8 && !str.empty()) str.pop_back();
        else if (event.text.unicode >= 32 && event.text.unicode < 128)
            str += (char)event.text.unicode;
    }

    void drawInputBox(float x, float y, float w, float h,
        const string& label, const string& value,
        bool active, bool masked = false) {
        Color bg = active ? Color(255, 255, 230) : Color(240, 240, 240);
        UIHelper::drawRect(window, x, y, w, h, bg, Color(100, 100, 100), 1.5f);
        UIHelper::drawText(window, font, label, x, y - 18, 13, Color(60, 60, 60));
        string dv = masked ? string(value.size(), '*') : value;
        dv += active ? "|" : "";
        UIHelper::drawText(window, font, dv, x + 5, y + 8, 15, Color(20, 20, 20));
    }

    // ── Sample menu seed ──────────────────────────────────────────────────
    void addSampleMenu() {
        menu.push_back(MenuItem(nextMenuId++, "Chicken Karahi", 850, "Main Course"));
        menu.push_back(MenuItem(nextMenuId++, "Mutton Karahi", 1200, "Main Course"));
        menu.push_back(MenuItem(nextMenuId++, "Beef Biryani", 450, "Main Course"));
        menu.push_back(MenuItem(nextMenuId++, "Chicken Tikka", 650, "Starter"));
        menu.push_back(MenuItem(nextMenuId++, "Seekh Kebab (6pc)", 480, "Starter"));
        menu.push_back(MenuItem(nextMenuId++, "Naan", 60, "Bread"));
        menu.push_back(MenuItem(nextMenuId++, "Tandoori Roti", 40, "Bread"));
        menu.push_back(MenuItem(nextMenuId++, "Raita", 80, "Side"));
        menu.push_back(MenuItem(nextMenuId++, "Lassi (Sweet)", 150, "Drink"));
        menu.push_back(MenuItem(nextMenuId++, "Cold Drink", 80, "Drink"));
        menu.push_back(MenuItem(nextMenuId++, "Gulab Jamun (4pc)", 200, "Dessert"));
        menu.push_back(MenuItem(nextMenuId++, "Kheer", 150, "Dessert"));
        FileHandler::saveMenu(menu);
    }

    void updateCustomerAfterPayment(const Order& ord) {
        for (auto& c : customers) {
            if (c.getName() == ord.getCustomerName()) {
                c.recordVisit(ord.getTotal());
                FileHandler::saveCustomers(customers);
                if (currentCustomer && currentCustomer->getName() == c.getName())
                    currentCustomer = &c;
                break;
            }
        }
    }

    // ═════════════════════════════════════════════════════════════════════
    //  SHARED DRAW HELPERS (top bar / nav bar / status)
    // ═════════════════════════════════════════════════════════════════════

    void drawStatusBar() {
        if (statusClock.getElapsedTime().asSeconds() > 3.0f) statusMessage = "";
        if (!statusMessage.empty()) {
            UIHelper::drawRect(window, 0, WINDOW_HEIGHT - 35, WINDOW_WIDTH, 35, Color(30, 30, 30, 220));
            UIHelper::drawText(window, font, statusMessage, 10, WINDOW_HEIGHT - 28, 16, Color::Yellow);
        }
    }

    void drawTopBar(const string& screenTitle) {
        UIHelper::drawRect(window, 0, 0, WINDOW_WIDTH, 60, Color(120, 20, 20));
        UIHelper::drawText(window, font, "Bundu Khan Restaurant", 15, 15, 22, Color(255, 220, 100));
        UIHelper::drawText(window, font, ">> " + screenTitle + " <<", 350, 18, 18, Color::White);
        if (currentRole == UserRole::STAFF && currentStaff) {
            UIHelper::drawText(window, font,
                "Staff: " + currentStaff->getName() + " (" + currentStaff->getRole() + ")",
                720, 10, 13, Color(200, 255, 200));
        }
        else if (currentRole == UserRole::CUSTOMER && currentCustomer) {
            UIHelper::drawText(window, font, "Customer: " + currentCustomer->getName(), 750, 10, 13, Color(200, 220, 255));
            UIHelper::drawText(window, font, "Visits: " + intToStr(currentCustomer->getVisits()), 750, 28, 12, Color(180, 200, 255));
        }
    }

    void drawNavBar() {
        float y = 65, h = 32, x = 5;
        if (currentRole == UserRole::STAFF) {
            float w = 87;
            vector<string> nav = { "Home","Menu","Orders","Billing","Reserve","Customers","Reports","Kitchen","Feedback","Queue" };
            vector<Screen> scr = { Screen::HOME,Screen::MENU,Screen::ORDER,Screen::BILLING,
                                  Screen::RESERVATION,Screen::CUSTOMERS,Screen::REPORTS,
                                  Screen::KITCHEN,Screen::FEEDBACK,Screen::QUEUE };
            for (int i = 0; i < (int)nav.size(); i++) {
                Color c = (currentScreen == scr[i]) ? Color(200, 150, 50) : Color(60, 60, 80);
                UIHelper::drawButton(window, font, nav[i], x + i * (w + 3), y, w, h, c, Color::White, 13);
            }
        }
        else {
            float w = 135;
            vector<string> nav = { "Home","View Menu","My Order","My Bill","Reserve","Feedback","Queue" };
            vector<Screen> scr = { Screen::HOME,Screen::MENU,Screen::ORDER,Screen::BILLING,
                                  Screen::RESERVATION,Screen::FEEDBACK,Screen::QUEUE };
            for (int i = 0; i < (int)nav.size(); i++) {
                Color c = (currentScreen == scr[i]) ? Color(20, 100, 200) : Color(40, 60, 100);
                UIHelper::drawButton(window, font, nav[i], x + i * (w + 3), y, w, h, c, Color::White, 13);
            }
        }
    }

    void handleNavBarClick(Event& event) {
        float y = 65, h = 32, x = 5;
        if (currentRole == UserRole::STAFF) {
            float w = 87;
            vector<Screen> scr = { Screen::HOME,Screen::MENU,Screen::ORDER,Screen::BILLING,
                                  Screen::RESERVATION,Screen::CUSTOMERS,Screen::REPORTS,
                                  Screen::KITCHEN,Screen::FEEDBACK,Screen::QUEUE };
            for (int i = 0; i < (int)scr.size(); i++)
                if (clicked(event, x + i * (w + 3), y, w, h)) currentScreen = scr[i];
        }
        else {
            float w = 135;
            vector<Screen> scr = { Screen::HOME,Screen::MENU,Screen::ORDER,Screen::BILLING,
                                  Screen::RESERVATION,Screen::FEEDBACK,Screen::QUEUE };
            for (int i = 0; i < (int)scr.size(); i++)
                if (clicked(event, x + i * (w + 3), y, w, h)) currentScreen = scr[i];
        }
    }

    // ═════════════════════════════════════════════════════════════════════
    //  SCREEN DRAW METHODS
    // ═════════════════════════════════════════════════════════════════════

    void drawLoginScreen() {
        UIHelper::drawRect(window, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT / 2, Color(180, 30, 30));
        UIHelper::drawRect(window, 0, WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT / 2, Color(20, 20, 40));
        UIHelper::drawRect(window, WINDOW_WIDTH / 2 - 200, 50, 400, 100, Color(0, 0, 0, 100));
        UIHelper::drawText(window, font, "BUNDU KHAN", WINDOW_WIDTH / 2 - 155, 65, 42, Color(255, 220, 80));
        UIHelper::drawText(window, font, "Restaurant Management System", WINDOW_WIDTH / 2 - 175, 118, 18, Color(230, 230, 230));

        float cx = WINDOW_WIDTH / 2 - 210, cy = 175;
        if (!showRegisterForm) {
            UIHelper::drawRect(window, cx - 10, cy - 10, 430, 360, Color(255, 255, 255, 220), Color(180, 30, 30), 2);
            UIHelper::drawText(window, font, "Login As:", cx + 10, cy + 2, 16, Color(80, 30, 30));
            Color sc = !loginAsCustomer ? Color(120, 20, 20) : Color(150, 150, 150);
            Color cc = loginAsCustomer ? Color(20, 60, 140) : Color(150, 150, 150);
            UIHelper::drawButton(window, font, "Staff", cx + 120, cy, 100, 30, sc, Color::White, 15);
            UIHelper::drawButton(window, font, "Customer", cx + 228, cy, 120, 30, cc, Color::White, 15);
            string title = loginAsCustomer ? "Customer Login" : "Staff Login";
            UIHelper::drawText(window, font, title, cx + 130, cy + 38, 22, loginAsCustomer ? Color(20, 60, 140) : Color(120, 20, 20));
            drawInputBox(cx + 10, cy + 80, 390, 38, "Username:", loginUsernameInput, typingUsername);
            drawInputBox(cx + 10, cy + 155, 390, 38, "Password:", loginPasswordInput, typingPassword, true);
            Color lc = loginAsCustomer ? Color(20, 60, 140) : Color(120, 20, 20);
            UIHelper::drawButton(window, font, "LOGIN", cx + 115, cy + 225, 180, 45, lc, Color::White, 20);
            if (loginAsCustomer) {
                UIHelper::drawText(window, font, "New customer? Register below:", cx + 80, cy + 285, 13, Color(80, 80, 80));
                UIHelper::drawButton(window, font, "Register as Customer", cx + 90, cy + 305, 220, 32, Color(0, 100, 60), Color::White, 14);
            }
            else {
                UIHelper::drawText(window, font, "Default: admin / admin123", cx + 90, cy + 285, 13, Color(100, 100, 100));
            }
        }
        else {
            UIHelper::drawRect(window, cx - 10, cy - 10, 430, 430, Color(255, 255, 255, 220), Color(0, 100, 60), 2);
            UIHelper::drawText(window, font, "New Customer Registration", cx + 50, cy + 2, 18, Color(0, 80, 50));
            drawInputBox(cx + 10, cy + 38, 390, 34, "Full Name:", regNameInput, typingRegName);
            drawInputBox(cx + 10, cy + 108, 390, 34, "Phone:", regPhoneInput, typingRegPhone);
            drawInputBox(cx + 10, cy + 178, 390, 34, "Username:", regUserInput, typingRegUser);
            drawInputBox(cx + 10, cy + 248, 390, 34, "Password:", regPassInput, typingRegPass, true);
            UIHelper::drawButton(window, font, "Create Account", cx + 100, cy + 308, 200, 40, Color(0, 120, 60), Color::White, 16);
            UIHelper::drawButton(window, font, "< Back to Login", cx + 100, cy + 360, 200, 32, Color(100, 100, 100), Color::White, 14);
        }
    }

    void drawHomeScreen() {
        drawTopBar("Dashboard");
        drawNavBar();
        float startY = 110;
        UIHelper::drawRect(window, 10, startY, WINDOW_WIDTH - 20, 60, Color(80, 30, 10));
        string welcome = (currentRole == UserRole::CUSTOMER && currentCustomer)
            ? "Welcome, " + currentCustomer->getName() + "! | Your visits: " + intToStr(currentCustomer->getVisits())
            : "Welcome to Bundu Khan Restaurant Management System";
        UIHelper::drawText(window, font, welcome, 25, startY + 18, 18, Color(255, 220, 100));

        float cardY = startY + 80, cardW = 200, cardH = 100, gap = 20;
        auto drawCard = [&](float x, Color col, string lbl, string val) {
            UIHelper::drawRect(window, x, cardY, cardW, cardH, col, Color::White, 1);
            UIHelper::drawText(window, font, lbl, x + 10, cardY + 10, 15, Color::White);
            UIHelper::drawText(window, font, val, x + 65, cardY + 40, 32, Color::Yellow);
            };
        drawCard(10, Color(40, 100, 160), "Active Orders", intToStr(orders.size()));
        drawCard(10 + cardW + gap, Color(40, 130, 60), "Total Tables", intToStr(TOTAL_TABLES));
        int res = 0; for (const auto& t : tables) if (t.isReserved()) res++;
        drawCard(10 + 2 * (cardW + gap), Color(160, 80, 0), "Reserved", intToStr(res));
        drawCard(10 + 3 * (cardW + gap), Color(120, 30, 120), "Customers", intToStr(customers.size()));
        drawCard(10 + 4 * (cardW + gap), Color(30, 80, 130), "Menu Items", intToStr(menu.size()));

        float todaySales = FileHandler::loadTodaysSales();
        UIHelper::drawRect(window, 10, cardY + cardH + 20, WINDOW_WIDTH - 20, 60, Color(50, 50, 50));
        UIHelper::drawText(window, font, "Today's Revenue: Rs." + floatToStr(todaySales), 20, cardY + cardH + 35, 22, Color(100, 255, 100));

        float btnY = cardY + cardH + 100;
        UIHelper::drawText(window, font, "Quick Actions:", 10, btnY, 18, Color(50, 50, 50));
        btnY += 28;
        if (currentRole == UserRole::STAFF) {
            UIHelper::drawButton(window, font, "New Order", 10, btnY, 160, 40, Color(40, 100, 160), Color::White);
            UIHelper::drawButton(window, font, "View Menu", 180, btnY, 160, 40, Color(40, 130, 60), Color::White);
            UIHelper::drawButton(window, font, "Generate Bill", 350, btnY, 160, 40, Color(160, 80, 0), Color::White);
            UIHelper::drawButton(window, font, "Add Customer", 520, btnY, 160, 40, Color(120, 30, 120), Color::White);
        }
        else {
            UIHelper::drawButton(window, font, "View Menu", 10, btnY, 160, 40, Color(40, 130, 60), Color::White);
            UIHelper::drawButton(window, font, "Place Order", 180, btnY, 160, 40, Color(40, 100, 160), Color::White);
            UIHelper::drawButton(window, font, "My Bill", 350, btnY, 160, 40, Color(160, 80, 0), Color::White);
            UIHelper::drawButton(window, font, "Reserve Table", 520, btnY, 160, 40, Color(20, 100, 180), Color::White);
        }
        UIHelper::drawButton(window, font, "LOGOUT", WINDOW_WIDTH - 130, WINDOW_HEIGHT - 50, 120, 35, Color(180, 20, 20), Color::White);
        if (!waitQueue.empty())
            UIHelper::drawText(window, font,
                "Waiting Queue: " + intToStr(waitQueue.size()) + " group(s) waiting",
                10, btnY + 60, 16, Color(180, 60, 0));
    }

    void drawMenuScreen() {
        drawTopBar("Menu Management");
        drawNavBar();
        float panelY = 105;
        UIHelper::drawRect(window, 5, panelY, 310, 300, Color(245, 245, 255), Color(100, 100, 200), 1);
        if (currentRole == UserRole::STAFF) {
            UIHelper::drawText(window, font, "Add New Item", 15, panelY + 5, 16, Color(30, 30, 100));
            drawInputBox(15, panelY + 35, 290, 32, "Item Name:", menuNameInput, typingMenuName);
            drawInputBox(15, panelY + 100, 290, 32, "Price (Rs):", menuPriceInput, typingMenuPrice);
            drawInputBox(15, panelY + 165, 290, 32, "Category:", menuCatInput, typingMenuCat);
            UIHelper::drawButton(window, font, "Add Item", 80, panelY + 210, 160, 35, Color(40, 130, 60), Color::White);
            UIHelper::drawRect(window, 5, panelY + 250, 310, 115, Color(255, 250, 235), Color(180, 120, 0), 1);
            UIHelper::drawText(window, font, "Update Item Price", 15, panelY + 255, 14, Color(100, 60, 0));
            if (selectedMenuItemForEdit >= 0 && selectedMenuItemForEdit < (int)menu.size()) {
                UIHelper::drawText(window, font, "Selected: " + menu[selectedMenuItemForEdit].getItemName(), 15, panelY + 272, 12, Color(60, 40, 0));
                drawInputBox(15, panelY + 290, 200, 30, "New Price:", menuEditPriceInput, typingMenuEditPrice);
                UIHelper::drawButton(window, font, "Update", 220, panelY + 290, 90, 30, Color(160, 100, 0), Color::White, 13);
            }
            else {
                UIHelper::drawText(window, font, "Click an item to select it", 15, panelY + 280, 12, Color(120, 100, 60));
            }
        }
        else {
            UIHelper::drawText(window, font, "Browse Our Menu", 60, panelY + 100, 18, Color(30, 60, 120));
            UIHelper::drawText(window, font, "Click any item to see details", 30, panelY + 130, 13, Color(80, 80, 120));
            UIHelper::drawText(window, font, "(Use 'My Order' to place an order)", 30, panelY + 155, 12, Color(100, 100, 150));
        }
        UIHelper::drawRect(window, 325, panelY, WINDOW_WIDTH - 335, 530, Color(250, 250, 250), Color(150, 150, 150), 1);
        UIHelper::drawText(window, font, "Current Menu (" + intToStr(menu.size()) + " items):", 335, panelY + 5, 15, Color(30, 30, 80));
        int visible = 16;
        float itemY = panelY + 28;
        for (int i = menuScrollOffset; i < (int)menu.size() && i < menuScrollOffset + visible; i++) {
            float rowY = itemY + (i - menuScrollOffset) * 30;
            Color rowBg = (selectedMenuItemForEdit == i) ? Color(255, 240, 180)
                : (i % 2 == 0) ? Color(240, 240, 255) : Color(255, 255, 255);
            UIHelper::drawRect(window, 326, rowY, WINDOW_WIDTH - 337, 28, rowBg);
            UIHelper::drawText(window, font, menu[i].getDisplayLine(), 330, rowY + 5, 13, Color(20, 20, 20));
            if (currentRole == UserRole::STAFF)
                UIHelper::drawButton(window, font, "Del", WINDOW_WIDTH - 60, rowY + 2, 40, 24, Color(180, 30, 30), Color::White, 12);
        }
        UIHelper::drawButton(window, font, "^ Up", WINDOW_WIDTH - 80, panelY + 465, 75, 28, Color(80, 80, 80), Color::White, 13);
        UIHelper::drawButton(window, font, "v Down", WINDOW_WIDTH - 80, panelY + 498, 75, 28, Color(80, 80, 80), Color::White, 13);
    }

    void drawOrderScreen() {
        drawTopBar("Order Management");
        drawNavBar();
        float y = 105;
        UIHelper::drawRect(window, 5, y, 320, 530, Color(245, 252, 245), Color(0, 120, 0), 1);
        UIHelper::drawText(window, font, "New Order Setup", 15, y + 5, 16, Color(0, 80, 0));
        string cust = (currentRole == UserRole::CUSTOMER && currentCustomer) ? currentCustomer->getName() : orderCustomerInput;
        drawInputBox(15, y + 35, 300, 32, "Customer Name:", cust, typingOrderCustomer);
        UIHelper::drawText(window, font, "Select Table:", 15, y + 85, 14, Color(50, 50, 50));
        for (int i = 0; i < TOTAL_TABLES; i++) {
            float tx = 15 + (i % 5) * 58, ty = y + 105 + (i / 5) * 38;
            bool res = tables[i].isReserved();
            Color tc = (selectedTableForOrder == i + 1) ? Color(0, 180, 0) : res ? Color(180, 50, 50) : Color(100, 150, 100);
            UIHelper::drawButton(window, font, "T" + intToStr(i + 1), tx, ty, 50, 30, tc, Color::White, 13);
        }
        UIHelper::drawText(window, font, "Select Item:", 15, y + 182, 14, Color(50, 50, 50));
        for (int i = orderMenuScroll; i < (int)menu.size() && i < orderMenuScroll + 8; i++) {
            float ry = y + 200 + (i - orderMenuScroll) * 26;
            Color rc = (selectedMenuItemIndex == i) ? Color(0, 180, 0, 80) : Color::Transparent;
            UIHelper::drawRect(window, 15, ry, 295, 24, rc);
            UIHelper::drawText(window, font, menu[i].getItemName() + " Rs." + floatToStr(menu[i].getPrice()), 17, ry + 4, 13, Color(10, 10, 10));
        }
        UIHelper::drawText(window, font, "Qty:", 15, y + 412, 13, Color(50, 50, 50));
        drawInputBox(55, y + 408, 60, 28, "", orderQtyStr, typingOrderQty);
        UIHelper::drawButton(window, font, "Add to Order", 125, y + 408, 140, 28, Color(0, 120, 0), Color::White, 13);
        UIHelper::drawButton(window, font, "CONFIRM ORDER", 20, y + 450, 280, 40, Color(20, 80, 20), Color::White, 16);
        UIHelper::drawButton(window, font, "^", 290, y + 200, 25, 25, Color(80, 80, 80), Color::White, 14);
        UIHelper::drawButton(window, font, "v", 290, y + 410, 25, 25, Color(80, 80, 80), Color::White, 14);
        UIHelper::drawRect(window, 335, y, WINDOW_WIDTH - 345, 530, Color(252, 252, 245), Color(150, 150, 0), 1);
        UIHelper::drawText(window, font,
            "Current Order" + (activeOrder ? " (Table " + intToStr(activeOrder->getTableNumber()) + ")" : ": None"),
            345, y + 5, 16, Color(80, 60, 0));
        if (activeOrder) {
            UIHelper::drawText(window, font, "Customer: " + activeOrder->getCustomerName(), 345, y + 30, 14, Color(40, 40, 40));
            float rowY = y + 55;
            for (const auto& oi : activeOrder->getItems()) {
                UIHelper::drawText(window, font,
                    oi.item.getItemName() + " x" + intToStr(oi.quantity) + " = Rs." + floatToStr(oi.getSubtotal()),
                    345, rowY, 14, Color(20, 20, 80));
                UIHelper::drawButton(window, font, "X", WINDOW_WIDTH - 55, rowY - 3, 30, 22, Color(180, 30, 30), Color::White, 12);
                rowY += 28;
            }
            UIHelper::drawRect(window, 335, y + 400, WINDOW_WIDTH - 345, 120, Color(240, 245, 220));
            UIHelper::drawText(window, font, "Subtotal: Rs." + floatToStr(activeOrder->getSubtotal()), 345, y + 410, 15, Color(30, 30, 30));
            UIHelper::drawText(window, font, "Tax(17%): Rs." + floatToStr(activeOrder->getTax()), 345, y + 430, 15, Color(30, 30, 30));
            UIHelper::drawText(window, font, "Service(5%): Rs." + floatToStr(activeOrder->getServiceCharge()), 345, y + 450, 15, Color(30, 30, 30));
            UIHelper::drawText(window, font, "TOTAL: Rs." + floatToStr(activeOrder->getTotal()), 345, y + 475, 18, Color(180, 0, 0));
        }
    }

    void drawBillingScreen() {
        drawTopBar("Billing System");
        drawNavBar();
        float y = 105;
        UIHelper::drawRect(window, 5, y, 340, 530, Color(252, 248, 240), Color(150, 100, 0), 1);
        UIHelper::drawText(window, font, "Unpaid Orders:", 15, y + 5, 16, Color(100, 60, 0));
        int unpaid = 0;
        for (int i = 0; i < (int)orders.size(); i++) {
            if (!orders[i].isPaid()) {
                if (currentRole == UserRole::CUSTOMER && currentCustomer &&
                    orders[i].getCustomerName() != currentCustomer->getName()) continue;
                float ry = y + 30 + unpaid * 50;
                Color bg = (selectedOrderForBill == i) ? Color(255, 230, 150) : Color(248, 245, 235);
                UIHelper::drawRect(window, 8, ry, 330, 46, bg, Color(180, 140, 0), 1);
                UIHelper::drawText(window, font,
                    "Order #" + intToStr(orders[i].getOrderId()) + " Table:" + intToStr(orders[i].getTableNumber()),
                    15, ry + 3, 14, Color(30, 30, 30));
                UIHelper::drawText(window, font,
                    orders[i].getCustomerName() + " | Rs." + floatToStr(orders[i].getTotal()),
                    15, ry + 22, 12, Color(80, 0, 0));
                UIHelper::drawButton(window, font, "Select", 275, ry + 10, 60, 26, Color(80, 80, 20), Color::White, 12);
                unpaid++;
            }
        }
        if (unpaid == 0)
            UIHelper::drawText(window, font, "No unpaid orders.", 60, y + 40, 16, Color(100, 100, 100));

        if (selectedOrderForBill < (int)orders.size()) {
            Order& ord = orders[selectedOrderForBill];
            UIHelper::drawRect(window, 355, y, WINDOW_WIDTH - 365, 530, Color(255, 255, 248), Color(0, 100, 0), 1);
            UIHelper::drawText(window, font, "INVOICE", 480, y + 5, 22, Color(20, 80, 20));
            UIHelper::drawText(window, font, "Bundu Khan Restaurant", 380, y + 30, 15, Color(80, 0, 0));
            UIHelper::drawText(window, font, "Order #" + intToStr(ord.getOrderId()) + "  Table:" + intToStr(ord.getTableNumber()), 380, y + 50, 13, Color(40, 40, 40));
            UIHelper::drawText(window, font, "Customer: " + ord.getCustomerName(), 380, y + 68, 13, Color(40, 40, 40));
            UIHelper::drawText(window, font, "Date: " + ord.getOrderDate(), 380, y + 86, 13, Color(40, 40, 40));
            UIHelper::drawRect(window, 360, y + 105, WINDOW_WIDTH - 375, 2, Color(100, 100, 100));
            float rowY = y + 112;
            for (const auto& oi : ord.getItems()) {
                UIHelper::drawText(window, font, oi.item.getItemName() + " x" + intToStr(oi.quantity), 380, rowY, 13, Color(20, 20, 20));
                UIHelper::drawText(window, font, "Rs." + floatToStr(oi.getSubtotal()), 700, rowY, 13, Color(80, 0, 0));
                rowY += 22;
            }
            UIHelper::drawRect(window, 360, rowY + 5, WINDOW_WIDTH - 375, 2, Color(100, 100, 100));
            rowY += 12;
            UIHelper::drawText(window, font, "Subtotal:", 380, rowY, 14, Color(20, 20, 20));
            UIHelper::drawText(window, font, "Rs." + floatToStr(ord.getSubtotal()), 680, rowY, 14, Color(20, 20, 20));
            UIHelper::drawText(window, font, "Tax (17%):", 380, rowY + 20, 14, Color(20, 20, 20));
            UIHelper::drawText(window, font, "Rs." + floatToStr(ord.getTax()), 680, rowY + 20, 14, Color(20, 20, 20));
            UIHelper::drawText(window, font, "Service (5%):", 380, rowY + 40, 14, Color(20, 20, 20));
            UIHelper::drawText(window, font, "Rs." + floatToStr(ord.getServiceCharge()), 680, rowY + 40, 14, Color(20, 20, 20));
            UIHelper::drawRect(window, 360, rowY + 62, WINDOW_WIDTH - 375, 3, Color(0, 100, 0));
            UIHelper::drawText(window, font, "GRAND TOTAL:", 380, rowY + 70, 18, Color(0, 80, 0));
            UIHelper::drawText(window, font, "Rs." + floatToStr(ord.getTotal()), 620, rowY + 70, 18, Color(180, 0, 0));
            if (!ord.isPaid()) {
                UIHelper::drawButton(window, font, "Pay CASH", 380, rowY + 110, 160, 40, Color(0, 130, 0), Color::White, 16);
                UIHelper::drawButton(window, font, "Pay CARD", 560, rowY + 110, 160, 40, Color(0, 80, 160), Color::White, 16);
            }
            else {
                UIHelper::drawText(window, font, "PAID - Thank you!", 400, rowY + 115, 20, Color(0, 150, 0));
            }
        }
    }

    void drawReservationScreen() {
        drawTopBar("Table Reservation");
        drawNavBar();
        float y = 105;
        UIHelper::drawRect(window, 5, y, 310, 275, Color(245, 245, 255), Color(0, 0, 180), 1);
        UIHelper::drawText(window, font, "Reserve a Table", 15, y + 5, 16, Color(0, 0, 130));
        string resName = (currentRole == UserRole::CUSTOMER && currentCustomer) ? currentCustomer->getName() : reserveNameInput;
        drawInputBox(15, y + 35, 290, 32, "Customer Name:", resName, typingResName);
        drawInputBox(15, y + 100, 290, 32, "Time Slot (e.g. 7PM):", reserveTimeInput, typingResTime);
        UIHelper::drawText(window, font, "Select Table:", 15, y + 148, 14, Color(50, 50, 50));
        for (int i = 0; i < TOTAL_TABLES; i++) {
            float tx = 15 + (i % 5) * 58, ty = y + 165 + (i / 5) * 38;
            Color tc = tables[i].isReserved() ? Color(180, 50, 50)
                : (selectedTableForRes == i + 1) ? Color(0, 120, 200) : Color(80, 80, 180);
            UIHelper::drawButton(window, font, "T" + intToStr(i + 1), tx, ty, 50, 30, tc, Color::White, 13);
        }
        UIHelper::drawButton(window, font, "Reserve Table", 60, y + 240, 190, 35, Color(0, 0, 150), Color::White);
        UIHelper::drawRect(window, 325, y, WINDOW_WIDTH - 335, 530, Color(248, 248, 255), Color(150, 150, 200), 1);
        UIHelper::drawText(window, font, "Current Reservations:", 335, y + 5, 16, Color(0, 0, 100));
        float rowY = y + 30; bool any = false;
        for (int i = 0; i < TOTAL_TABLES; i++) {
            if (tables[i].isReserved()) {
                any = true;
                UIHelper::drawRect(window, 328, rowY, WINDOW_WIDTH - 340, 55, Color(220, 220, 255), Color(0, 0, 200), 1);
                UIHelper::drawText(window, font, "Table " + intToStr(tables[i].getTableNum()) + " (Cap:" + intToStr(tables[i].getCapacity()) + ")", 335, rowY + 3, 14, Color(0, 0, 100));
                UIHelper::drawText(window, font, "Reserved for: " + tables[i].getReservedFor() + " at " + tables[i].getReservationTime(), 335, rowY + 22, 13, Color(50, 50, 50));
                if (currentRole == UserRole::STAFF)
                    UIHelper::drawButton(window, font, "Cancel", WINDOW_WIDTH - 90, rowY + 12, 70, 28, Color(180, 30, 30), Color::White, 13);
                rowY += 63;
            }
        }
        if (!any)
            UIHelper::drawText(window, font, "No current reservations.", 400, y + 40, 16, Color(120, 120, 120));
    }

    void drawCustomerScreen() {
        drawTopBar("Customer Management");
        drawNavBar();
        float y = 105;
        if (currentRole == UserRole::STAFF) {
            UIHelper::drawRect(window, 5, y, 310, 200, Color(252, 245, 255), Color(120, 0, 120), 1);
            UIHelper::drawText(window, font, "Add New Customer", 15, y + 5, 16, Color(80, 0, 80));
            drawInputBox(15, y + 35, 290, 32, "Customer Name:", custNameInput, typingCustName);
            drawInputBox(15, y + 100, 290, 32, "Phone Number:", custPhoneInput, typingCustPhone);
            UIHelper::drawButton(window, font, "Add Customer", 60, y + 155, 190, 35, Color(100, 0, 100), Color::White);
        }
        else {
            UIHelper::drawRect(window, 5, y, 310, 130, Color(230, 240, 255), Color(0, 60, 140), 1);
            UIHelper::drawText(window, font, "Your Profile", 80, y + 10, 16, Color(0, 40, 100));
            if (currentCustomer) {
                UIHelper::drawText(window, font, "Name:   " + currentCustomer->getName(), 15, y + 35, 14, Color(20, 20, 80));
                UIHelper::drawText(window, font, "Phone:  " + currentCustomer->getPhone(), 15, y + 55, 14, Color(20, 20, 80));
                UIHelper::drawText(window, font, "Visits: " + intToStr(currentCustomer->getVisits()), 15, y + 75, 14, Color(20, 20, 80));
                UIHelper::drawText(window, font, "Total Spent: Rs." + floatToStr(currentCustomer->getTotalSpent()), 15, y + 95, 14, Color(20, 20, 80));
            }
        }
        UIHelper::drawRect(window, 325, y, WINDOW_WIDTH - 335, 530, Color(252, 248, 255), Color(150, 0, 150), 1);
        UIHelper::drawText(window, font, "Registered Customers (" + intToStr(customers.size()) + "):", 335, y + 5, 15, Color(80, 0, 80));
        int visible = 15; float rowY = y + 28;
        for (int i = custScrollOffset; i < (int)customers.size() && i < custScrollOffset + visible; i++) {
            Color bg = (i % 2 == 0) ? Color(245, 235, 255) : Color(255, 255, 255);
            UIHelper::drawRect(window, 327, rowY, WINDOW_WIDTH - 338, 32, bg);
            UIHelper::drawText(window, font, customers[i].getSummary(), 332, rowY + 7, 13, Color(20, 20, 20));
            if (customers[i].getFeedback() > 0)
                UIHelper::drawText(window, font, "Rating: " + intToStr(customers[i].getFeedback()) + "/10", WINDOW_WIDTH - 150, rowY + 7, 12, Color(0, 100, 0));
            rowY += 34;
        }
        UIHelper::drawButton(window, font, "^ Up", WINDOW_WIDTH - 80, y + 490, 75, 30, Color(80, 80, 80), Color::White, 13);
        UIHelper::drawButton(window, font, "v Down", WINDOW_WIDTH - 80, y + 525, 75, 30, Color(80, 80, 80), Color::White, 13);
    }

    void drawReportsScreen() {
        drawTopBar("Daily Reports");
        drawNavBar();
        if (currentRole == UserRole::CUSTOMER) {
            UIHelper::drawText(window, font, "Access Denied", 400, 300, 30, Color(180, 0, 0));
            UIHelper::drawText(window, font, "This screen is for staff only.", 320, 350, 18, Color(120, 0, 0));
            return;
        }
        float y = 110;
        UIHelper::drawRect(window, 10, y, WINDOW_WIDTH - 20, 510, Color(245, 255, 245), Color(0, 120, 0), 2);
        UIHelper::drawText(window, font, "Daily Sales Report - " + getTodayDate(), 30, y + 10, 20, Color(0, 80, 0));
        float todaySales = FileHandler::loadTodaysSales();
        int   todayOrders = FileHandler::loadTodaysOrderCount();
        UIHelper::drawRect(window, 20, y + 45, 300, 80, Color(0, 120, 0, 30));
        UIHelper::drawText(window, font, "Total Revenue Today:", 30, y + 55, 16, Color(20, 20, 20));
        UIHelper::drawText(window, font, "Rs. " + floatToStr(todaySales), 30, y + 80, 24, Color(0, 130, 0));
        UIHelper::drawRect(window, 340, y + 45, 300, 80, Color(0, 0, 150, 30));
        UIHelper::drawText(window, font, "Total Orders Today:", 350, y + 55, 16, Color(20, 20, 20));
        UIHelper::drawText(window, font, intToStr(todayOrders), 350, y + 80, 24, Color(0, 0, 150));
        UIHelper::drawRect(window, 660, y + 45, 300, 80, Color(150, 0, 0, 30));
        UIHelper::drawText(window, font, "Total Customers:", 670, y + 55, 16, Color(20, 20, 20));
        UIHelper::drawText(window, font, intToStr(customers.size()), 670, y + 80, 24, Color(150, 0, 0));
        UIHelper::drawText(window, font, "Active Orders Details:", 30, y + 145, 17, Color(30, 30, 80));
        map<string, int> itemCount;
        for (const auto& ord : orders)
            for (const auto& oi : ord.getItems())
                itemCount[oi.item.getItemName()] += oi.quantity;
        float rowY = y + 170;
        if (!orders.empty()) {
            for (const auto& ord : orders) {
                UIHelper::drawText(window, font,
                    "Order #" + intToStr(ord.getOrderId()) + " | Table " + intToStr(ord.getTableNumber()) +
                    " | " + ord.getCustomerName() + " | Rs." + floatToStr(ord.getTotal()) +
                    " | " + statusToStr(ord.getStatus()) + (ord.isPaid() ? " [PAID]" : " [UNPAID]"),
                    30, rowY, 13, Color(20, 20, 60));
                rowY += 22;
                if (rowY > y + 360) break;
            }
        }
        else { UIHelper::drawText(window, font, "No active orders.", 30, rowY, 15, Color(100, 100, 100)); }
        UIHelper::drawText(window, font, "Most Ordered Items (Active):", 30, y + 370, 16, Color(80, 0, 0));
        float miY = y + 395;
        for (const auto& p : itemCount) {
            UIHelper::drawText(window, font, p.first + ": " + intToStr(p.second) + " portions", 30, miY, 14, Color(30, 30, 30));
            miY += 22; if (miY > y + 490) break;
        }
        UIHelper::drawText(window, font, "Total Feedback: " + intToStr(feedbacks.size()), 600, y + 370, 14, Color(0, 80, 100));
        if (!feedbacks.empty()) {
            float avg = 0; for (const auto& f : feedbacks) avg += f.rating;
            avg /= feedbacks.size();
            UIHelper::drawText(window, font, "Avg Rating: " + floatToStr(avg) + "/10", 600, y + 395, 14, Color(0, 120, 0));
        }
    }

    void drawKitchenScreen() {
        drawTopBar("Kitchen Order Tracking");
        drawNavBar();
        if (currentRole == UserRole::CUSTOMER) {
            UIHelper::drawText(window, font, "Access Denied", 400, 300, 30, Color(180, 0, 0));
            UIHelper::drawText(window, font, "Kitchen screen is for staff only.", 290, 350, 18, Color(120, 0, 0));
            return;
        }
        float y = 105;
        UIHelper::drawText(window, font, "Order Status: Pending -> Preparing -> Ready -> Served", 15, y + 5, 14, Color(80, 40, 0));
        float rowY = y + 35;
        for (int i = kitchenScrollOffset; i < (int)orders.size() && i < kitchenScrollOffset + 8; i++) {
            Order& ord = orders[i];
            Color bg;
            switch (ord.getStatus()) {
            case OrderStatus::PENDING:   bg = Color(255, 240, 180); break;
            case OrderStatus::PREPARING: bg = Color(180, 230, 255); break;
            case OrderStatus::READY:     bg = Color(180, 255, 180); break;
            default:                     bg = Color(220, 220, 220); break;
            }
            UIHelper::drawRect(window, 5, rowY, WINDOW_WIDTH - 10, 58, bg, Color(100, 100, 100), 1);
            UIHelper::drawText(window, font,
                "Order #" + intToStr(ord.getOrderId()) + "  Table:" + intToStr(ord.getTableNumber()) + "  Customer: " + ord.getCustomerName(),
                15, rowY + 3, 15, Color(20, 20, 20));
            string itemsStr = "Items: ";
            for (const auto& oi : ord.getItems()) itemsStr += oi.item.getItemName() + "x" + intToStr(oi.quantity) + " ";
            UIHelper::drawText(window, font, itemsStr, 15, rowY + 22, 12, Color(40, 40, 40));
            UIHelper::drawRect(window, WINDOW_WIDTH - 290, rowY + 8, 130, 30, Color(50, 50, 50));
            UIHelper::drawText(window, font, "Status: " + statusToStr(ord.getStatus()), WINDOW_WIDTH - 288, rowY + 12, 14, Color::Yellow);
            if (ord.getStatus() != OrderStatus::SERVED)
                UIHelper::drawButton(window, font, "Advance->", WINDOW_WIDTH - 150, rowY + 8, 140, 30, Color(30, 100, 30), Color::White, 14);
            rowY += 65;
        }
        if (orders.empty())
            UIHelper::drawText(window, font, "No active orders in kitchen.", 300, y + 60, 18, Color(120, 120, 120));
        UIHelper::drawButton(window, font, "^ Up", 10, WINDOW_HEIGHT - 80, 100, 30, Color(80, 80, 80), Color::White);
        UIHelper::drawButton(window, font, "v Down", 120, WINDOW_HEIGHT - 80, 100, 30, Color(80, 80, 80), Color::White);
    }

    void drawFeedbackScreen() {
        drawTopBar("Customer Feedback & Ratings");
        drawNavBar();
        float y = 105;
        UIHelper::drawRect(window, 5, y, 310, 280, Color(245, 255, 245), Color(0, 130, 0), 1);
        UIHelper::drawText(window, font, "Submit Feedback", 15, y + 5, 16, Color(0, 80, 0));
        string fbName = (currentRole == UserRole::CUSTOMER && currentCustomer) ? currentCustomer->getName() : feedbackCustInput;
        drawInputBox(15, y + 35, 290, 32, "Customer Name:", fbName, typingFbCust);
        drawInputBox(15, y + 100, 290, 32, "Rating (1-10):", feedbackRatingInput, typingFbRating);
        drawInputBox(15, y + 165, 290, 32, "Comment:", feedbackCommentInput, typingFbComment);
        UIHelper::drawButton(window, font, "Submit", 80, y + 220, 160, 38, Color(0, 120, 0), Color::White);
        UIHelper::drawRect(window, 325, y, WINDOW_WIDTH - 335, 530, Color(248, 255, 248), Color(0, 150, 0), 1);
        UIHelper::drawText(window, font, "Recent Feedback (" + intToStr(feedbacks.size()) + " entries):", 335, y + 5, 15, Color(0, 80, 0));
        float rowY = y + 30;
        for (const auto& fb : feedbacks) {
            Color rc = (fb.rating >= 8) ? Color(0, 150, 0) : (fb.rating >= 5) ? Color(150, 130, 0) : Color(180, 0, 0);
            UIHelper::drawRect(window, 328, rowY, WINDOW_WIDTH - 340, 50, Color(240, 255, 240), Color(0, 130, 0), 1);
            UIHelper::drawText(window, font, fb.customerName + " - Rating: " + intToStr(fb.rating) + "/10", 335, rowY + 3, 14, rc);
            UIHelper::drawText(window, font, "Comment: " + fb.comment + " | " + fb.date, 335, rowY + 22, 12, Color(50, 50, 50));
            rowY += 57;
            if (rowY > y + 510) break;
        }
        if (feedbacks.empty())
            UIHelper::drawText(window, font, "No feedback yet.", 400, y + 50, 16, Color(120, 120, 120));
    }

    void drawQueueScreen() {
        drawTopBar("Waiting Queue");
        drawNavBar();
        float y = 105;
        UIHelper::drawRect(window, 5, y, 310, 280, Color(255, 248, 240), Color(180, 80, 0), 1);
        UIHelper::drawText(window, font, "Add to Waiting Queue", 15, y + 5, 15, Color(120, 50, 0));
        drawInputBox(15, y + 35, 290, 32, "Party Name:", queueNameInput, typingQueueName);
        drawInputBox(15, y + 100, 290, 32, "Party Size:", queueSizeInput, typingQueueSize);
        drawInputBox(15, y + 165, 290, 32, "Arrival Time:", queueTimeInput, typingQueueTime);
        UIHelper::drawButton(window, font, "Add to Queue", 60, y + 220, 190, 38, Color(160, 80, 0), Color::White);
        UIHelper::drawRect(window, 325, y, WINDOW_WIDTH - 335, 530, Color(255, 252, 245), Color(180, 100, 0), 1);
        UIHelper::drawText(window, font, "Current Queue (" + intToStr(waitQueue.size()) + " groups):", 335, y + 5, 15, Color(120, 50, 0));
        float rowY = y + 30;
        for (int i = 0; i < (int)waitQueue.size(); i++) {
            Color bg = (i % 2 == 0) ? Color(255, 240, 220) : Color(255, 250, 240);
            UIHelper::drawRect(window, 328, rowY, WINDOW_WIDTH - 340, 50, bg, Color(200, 140, 0), 1);
            UIHelper::drawText(window, font,
                intToStr(i + 1) + ". " + waitQueue[i].name + " | Party: " + intToStr(waitQueue[i].partySize),
                335, rowY + 3, 14, Color(80, 40, 0));
            UIHelper::drawText(window, font, "Arrived: " + waitQueue[i].arrivalTime, 335, rowY + 24, 12, Color(100, 60, 0));
            UIHelper::drawButton(window, font, "Seat", WINDOW_WIDTH - 90, rowY + 10, 60, 28, Color(0, 120, 60), Color::White, 13);
            rowY += 57;
            if (rowY > y + 510) break;
        }
        if (waitQueue.empty())
            UIHelper::drawText(window, font, "No groups waiting.", 400, y + 40, 16, Color(120, 120, 120));
    }

    // ═════════════════════════════════════════════════════════════════════
    //  EVENT HANDLERS
    // ═════════════════════════════════════════════════════════════════════

    void handleLoginEvents(Event& event) {
        float cx = WINDOW_WIDTH / 2 - 210, cy = 175;
        if (!showRegisterForm) {
            if (clicked(event, cx + 120, cy, 100, 30)) { loginAsCustomer = false; loginUsernameInput = ""; loginPasswordInput = ""; }
            if (clicked(event, cx + 228, cy, 120, 30)) { loginAsCustomer = true;  loginUsernameInput = ""; loginPasswordInput = ""; }
            if (clicked(event, cx + 10, cy + 80, 390, 38)) { typingUsername = true;  typingPassword = false; }
            if (clicked(event, cx + 10, cy + 155, 390, 38)) { typingUsername = false; typingPassword = true; }
            handleTextInput(event, loginUsernameInput, typingUsername);
            handleTextInput(event, loginPasswordInput, typingPassword);
            if (clicked(event, cx + 115, cy + 225, 180, 45)) attemptLogin();
            if (event.type == Event::KeyPressed && event.key.code == Keyboard::Return) attemptLogin();
            if (loginAsCustomer && clicked(event, cx + 90, cy + 305, 220, 32)) {
                showRegisterForm = true; typingUsername = false; typingPassword = false;
            }
        }
        else {
            if (clicked(event, cx + 10, cy + 38, 390, 34)) { typingRegName = true;  typingRegPhone = typingRegUser = typingRegPass = false; }
            if (clicked(event, cx + 10, cy + 108, 390, 34)) { typingRegPhone = true; typingRegName = typingRegUser = typingRegPass = false; }
            if (clicked(event, cx + 10, cy + 178, 390, 34)) { typingRegUser = true;  typingRegName = typingRegPhone = typingRegPass = false; }
            if (clicked(event, cx + 10, cy + 248, 390, 34)) { typingRegPass = true;  typingRegName = typingRegPhone = typingRegUser = false; }
            handleTextInput(event, regNameInput, typingRegName);
            handleTextInput(event, regPhoneInput, typingRegPhone);
            handleTextInput(event, regUserInput, typingRegUser);
            handleTextInput(event, regPassInput, typingRegPass);
            if (clicked(event, cx + 100, cy + 308, 200, 40)) {
                if (!regNameInput.empty() && !regPhoneInput.empty() && !regUserInput.empty() && !regPassInput.empty()) {
                    bool taken = false;
                    for (auto& c : customers) if (c.getUsername() == regUserInput) { taken = true; break; }
                    if (!taken) {
                        customers.push_back(Customer(nextCustomerId++, regNameInput, regPhoneInput, regUserInput, regPassInput));
                        FileHandler::saveCustomers(customers);
                        showStatus("Account created! You can now login as " + regUserInput);
                        regNameInput = regPhoneInput = regUserInput = regPassInput = "";
                        showRegisterForm = false; loginAsCustomer = true;
                    }
                    else { showStatus("Username already taken. Choose another."); }
                }
                else { showStatus("Fill all fields to register."); }
            }
            if (clicked(event, cx + 100, cy + 360, 200, 32)) {
                showRegisterForm = false;
                regNameInput = regPhoneInput = regUserInput = regPassInput = "";
            }
        }
    }

    void attemptLogin() {
        if (!loginAsCustomer) {
            for (auto& s : staff) {
                if (s.getUsername() == loginUsernameInput && s.checkPassword(loginPasswordInput)) {
                    loggedIn = true; currentRole = UserRole::STAFF;
                    currentStaff = &s; currentCustomer = nullptr;
                    currentScreen = Screen::HOME;
                    showStatus("Welcome, " + s.getName() + "! (Staff)");
                    loginUsernameInput = loginPasswordInput = "";
                    return;
                }
            }
            showStatus("Invalid staff credentials. Try again.");
            loginPasswordInput = "";
        }
        else {
            for (auto& c : customers) {
                if (c.hasLogin() && c.getUsername() == loginUsernameInput && c.checkPassword(loginPasswordInput)) {
                    loggedIn = true; currentRole = UserRole::CUSTOMER;
                    currentCustomer = &c; currentStaff = nullptr;
                    currentScreen = Screen::HOME;
                    showStatus("Welcome back, " + c.getName() + "!");
                    loginUsernameInput = loginPasswordInput = "";
                    return;
                }
            }
            showStatus("Invalid customer credentials. Register first.");
            loginPasswordInput = "";
        }
    }

    void handleHomeEvents(Event& event) {
        handleNavBarClick(event);
        float startY = 110, cardY = startY + 80, cardH = 100, btnY = cardY + cardH + 128;
        if (currentRole == UserRole::STAFF) {
            if (clicked(event, 10, btnY, 160, 40)) currentScreen = Screen::ORDER;
            if (clicked(event, 180, btnY, 160, 40)) currentScreen = Screen::MENU;
            if (clicked(event, 350, btnY, 160, 40)) currentScreen = Screen::BILLING;
            if (clicked(event, 520, btnY, 160, 40)) currentScreen = Screen::CUSTOMERS;
        }
        else {
            if (clicked(event, 10, btnY, 160, 40)) currentScreen = Screen::MENU;
            if (clicked(event, 180, btnY, 160, 40)) currentScreen = Screen::ORDER;
            if (clicked(event, 350, btnY, 160, 40)) currentScreen = Screen::BILLING;
            if (clicked(event, 520, btnY, 160, 40)) currentScreen = Screen::RESERVATION;
        }
        if (clicked(event, WINDOW_WIDTH - 130, WINDOW_HEIGHT - 50, 120, 35)) {
            loggedIn = false; currentRole = UserRole::NONE;
            currentStaff = nullptr; currentCustomer = nullptr;
            currentScreen = Screen::LOGIN; loginAsCustomer = false;
            showStatus("Logged out successfully.");
        }
    }

    void handleMenuEvents(Event& event) {
        handleNavBarClick(event);
        float panelY = 105;
        if (currentRole == UserRole::STAFF) {
            if (clicked(event, 15, panelY + 35, 290, 32)) { typingMenuName = true;  typingMenuPrice = typingMenuCat = typingMenuEditPrice = false; }
            if (clicked(event, 15, panelY + 100, 290, 32)) { typingMenuPrice = true; typingMenuName = typingMenuCat = typingMenuEditPrice = false; }
            if (clicked(event, 15, panelY + 165, 290, 32)) { typingMenuCat = true;   typingMenuName = typingMenuPrice = typingMenuEditPrice = false; }
            if (clicked(event, 15, panelY + 290, 200, 30)) { typingMenuEditPrice = true; typingMenuName = typingMenuPrice = typingMenuCat = false; }
            handleTextInput(event, menuNameInput, typingMenuName);
            handleTextInput(event, menuPriceInput, typingMenuPrice);
            handleTextInput(event, menuCatInput, typingMenuCat);
            handleTextInput(event, menuEditPriceInput, typingMenuEditPrice);
            if (clicked(event, 80, panelY + 210, 160, 35)) {
                if (!menuNameInput.empty() && !menuPriceInput.empty() && !menuCatInput.empty()) {
                    menu.push_back(MenuItem(nextMenuId++, menuNameInput, stof(menuPriceInput), menuCatInput));
                    FileHandler::saveMenu(menu);
                    showStatus("Added: " + menuNameInput);
                    menuNameInput = menuPriceInput = menuCatInput = "";
                }
                else { showStatus("Fill all fields."); }
            }
            if (selectedMenuItemForEdit >= 0 && selectedMenuItemForEdit < (int)menu.size()) {
                if (clicked(event, 220, panelY + 290, 90, 30)) {
                    if (!menuEditPriceInput.empty()) {
                        float np = stof(menuEditPriceInput);
                        menu[selectedMenuItemForEdit].setPrice(np);
                        FileHandler::saveMenu(menu);
                        showStatus("Price updated: " + menu[selectedMenuItemForEdit].getItemName() + " -> Rs." + floatToStr(np));
                        menuEditPriceInput = ""; selectedMenuItemForEdit = -1;
                    }
                    else { showStatus("Enter new price first."); }
                }
            }
        }
        int visible = 16; float itemY = panelY + 28;
        for (int i = menuScrollOffset; i < (int)menu.size() && i < menuScrollOffset + visible; i++) {
            float rowY = itemY + (i - menuScrollOffset) * 30;
            if (clicked(event, 326, rowY, WINDOW_WIDTH - 337 - 45, 28)) {
                selectedMenuItemForEdit = (selectedMenuItemForEdit == i) ? -1 : i;
                menuEditPriceInput = ""; typingMenuEditPrice = false;
            }
            if (currentRole == UserRole::STAFF) {
                if (clicked(event, WINDOW_WIDTH - 60, rowY + 2, 40, 24)) {
                    showStatus("Removed: " + menu[i].getItemName());
                    if (selectedMenuItemForEdit == i) selectedMenuItemForEdit = -1;
                    menu.erase(menu.begin() + i);
                    FileHandler::saveMenu(menu);
                    break;
                }
            }
        }
        if (clicked(event, WINDOW_WIDTH - 80, panelY + 465, 75, 28) && menuScrollOffset > 0) menuScrollOffset--;
        if (clicked(event, WINDOW_WIDTH - 80, panelY + 498, 75, 28) && menuScrollOffset < (int)menu.size() - 16) menuScrollOffset++;
        if (event.type == Event::MouseWheelScrolled) {
            if (event.mouseWheelScroll.delta > 0 && menuScrollOffset > 0) menuScrollOffset--;
            if (event.mouseWheelScroll.delta < 0 && menuScrollOffset < (int)menu.size() - 16) menuScrollOffset++;
        }
    }

    void handleOrderEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105;
        if (clicked(event, 15, y + 35, 300, 32)) typingOrderCustomer = true;
        else if (event.type == Event::MouseButtonPressed) typingOrderCustomer = false;
        if (currentRole == UserRole::CUSTOMER && currentCustomer) orderCustomerInput = currentCustomer->getName();
        else handleTextInput(event, orderCustomerInput, typingOrderCustomer);
        for (int i = 0; i < TOTAL_TABLES; i++) {
            float tx = 15 + (i % 5) * 58, ty = y + 105 + (i / 5) * 38;
            if (clicked(event, tx, ty, 50, 30)) selectedTableForOrder = i + 1;
        }
        for (int i = orderMenuScroll; i < (int)menu.size() && i < orderMenuScroll + 8; i++) {
            float ry = y + 200 + (i - orderMenuScroll) * 26;
            if (clicked(event, 15, ry, 295, 24)) selectedMenuItemIndex = i;
        }
        if (clicked(event, 55, y + 408, 60, 28)) typingOrderQty = true;
        else if (event.type == Event::MouseButtonPressed) typingOrderQty = false;
        handleTextInput(event, orderQtyStr, typingOrderQty);
        string custName = (currentRole == UserRole::CUSTOMER && currentCustomer) ? currentCustomer->getName() : orderCustomerInput;
        if (clicked(event, 125, y + 408, 140, 28)) {
            if (custName.empty()) { showStatus("Enter customer name first!"); return; }
            if (!activeOrder) activeOrder = new Order(nextOrderId++, selectedTableForOrder, custName);
            if (selectedMenuItemIndex >= 0 && selectedMenuItemIndex < (int)menu.size()) {
                int qty = orderQtyStr.empty() ? 1 : max(1, stoi(orderQtyStr));
                activeOrder->addItem(menu[selectedMenuItemIndex], qty);
                showStatus("Added: " + menu[selectedMenuItemIndex].getItemName());
            }
        }
        if (activeOrder) {
            float rowY = y + 55;
            const auto& items = activeOrder->getItems();
            for (int i = 0; i < (int)items.size(); i++) {
                if (clicked(event, WINDOW_WIDTH - 55, rowY - 3, 30, 22)) {
                    const_cast<Order*>(activeOrder)->removeItem(items[i].item.getItemId());
                    showStatus("Item removed."); break;
                }
                rowY += 28;
            }
        }
        if (clicked(event, 20, y + 450, 280, 40)) {
            if (activeOrder && !activeOrder->getItems().empty()) {
                orders.push_back(*activeOrder);
                showStatus("Order #" + intToStr(activeOrder->getOrderId()) + " confirmed!");
                delete activeOrder; activeOrder = nullptr; orderCustomerInput = "";
            }
            else { showStatus("Add at least one item."); }
        }
        if (clicked(event, 290, y + 200, 25, 25) && orderMenuScroll > 0) orderMenuScroll--;
        if (clicked(event, 290, y + 410, 25, 25) && orderMenuScroll < (int)menu.size() - 8) orderMenuScroll++;
    }

    void handleBillingEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105;
        int unpaid = 0;
        for (int i = 0; i < (int)orders.size(); i++) {
            if (!orders[i].isPaid()) {
                if (currentRole == UserRole::CUSTOMER && currentCustomer &&
                    orders[i].getCustomerName() != currentCustomer->getName()) continue;
                float ry = y + 30 + unpaid * 50;
                if (clicked(event, 275, ry + 10, 60, 26)) selectedOrderForBill = i;
                unpaid++;
            }
        }
        if (selectedOrderForBill < (int)orders.size()) {
            Order& ord = orders[selectedOrderForBill];
            if (!ord.isPaid()) {
                float rowY = y + 112;
                for (const auto& oi : ord.getItems()) rowY += 22;
                rowY += 12;
                if (clicked(event, 380, rowY + 110, 160, 40)) {
                    ord.markPaid(PaymentMethod::CASH);
                    FileHandler::saveOrderHistory(ord);
                    updateCustomerAfterPayment(ord);
                    showStatus("Cash payment received. Order #" + intToStr(ord.getOrderId()) + " paid.");
                }
                if (clicked(event, 560, rowY + 110, 160, 40)) {
                    ord.markPaid(PaymentMethod::CARD);
                    FileHandler::saveOrderHistory(ord);
                    updateCustomerAfterPayment(ord);
                    showStatus("Card payment received. Order #" + intToStr(ord.getOrderId()) + " paid.");
                }
            }
        }
    }

    void handleReservationEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105;
        if (currentRole == UserRole::CUSTOMER && currentCustomer) reserveNameInput = currentCustomer->getName();
        else {
            if (clicked(event, 15, y + 35, 290, 32)) { typingResName = true;  typingResTime = false; }
            handleTextInput(event, reserveNameInput, typingResName);
        }
        if (clicked(event, 15, y + 100, 290, 32)) { typingResName = false; typingResTime = true; }
        handleTextInput(event, reserveTimeInput, typingResTime);
        for (int i = 0; i < TOTAL_TABLES; i++) {
            float tx = 15 + (i % 5) * 58, ty = y + 165 + (i / 5) * 38;
            if (clicked(event, tx, ty, 50, 30)) selectedTableForRes = i + 1;
        }
        if (clicked(event, 60, y + 240, 190, 35)) {
            string name = (currentRole == UserRole::CUSTOMER && currentCustomer) ? currentCustomer->getName() : reserveNameInput;
            if (!name.empty() && !reserveTimeInput.empty()) {
                tables[selectedTableForRes - 1].reserve(name, reserveTimeInput);
                showStatus("Table " + intToStr(selectedTableForRes) + " reserved for " + name);
                reserveNameInput = reserveTimeInput = "";
            }
            else { showStatus("Fill in name and time slot."); }
        }
        if (currentRole == UserRole::STAFF) {
            float rowY = y + 30;
            for (int i = 0; i < TOTAL_TABLES; i++) {
                if (tables[i].isReserved()) {
                    if (clicked(event, WINDOW_WIDTH - 90, rowY + 12, 70, 28)) {
                        showStatus("Reservation cancelled for Table " + intToStr(tables[i].getTableNum()));
                        tables[i].cancelReservation();
                    }
                    rowY += 63;
                }
            }
        }
    }

    void handleCustomerEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105;
        if (currentRole == UserRole::STAFF) {
            if (clicked(event, 15, y + 35, 290, 32)) { typingCustName = true;  typingCustPhone = false; }
            if (clicked(event, 15, y + 100, 290, 32)) { typingCustName = false; typingCustPhone = true; }
            handleTextInput(event, custNameInput, typingCustName);
            handleTextInput(event, custPhoneInput, typingCustPhone);
            if (clicked(event, 60, y + 155, 190, 35)) {
                if (!custNameInput.empty() && !custPhoneInput.empty()) {
                    customers.push_back(Customer(nextCustomerId++, custNameInput, custPhoneInput));
                    FileHandler::saveCustomers(customers);
                    showStatus("Customer added: " + custNameInput);
                    custNameInput = custPhoneInput = "";
                }
                else { showStatus("Enter both name and phone."); }
            }
        }
        if (clicked(event, WINDOW_WIDTH - 80, y + 490, 75, 30) && custScrollOffset > 0) custScrollOffset--;
        if (clicked(event, WINDOW_WIDTH - 80, y + 525, 75, 30) && custScrollOffset < (int)customers.size() - 15) custScrollOffset++;
    }

    void handleReportEvents(Event& event) { handleNavBarClick(event); }

    void handleKitchenEvents(Event& event) {
        handleNavBarClick(event);
        if (currentRole != UserRole::STAFF) return;
        float y = 105, rowY = y + 35;
        for (int i = kitchenScrollOffset; i < (int)orders.size() && i < kitchenScrollOffset + 8; i++) {
            if (orders[i].getStatus() != OrderStatus::SERVED) {
                if (clicked(event, WINDOW_WIDTH - 150, rowY + 8, 140, 30)) {
                    orders[i].advanceStatus();
                    showStatus("Order #" + intToStr(orders[i].getOrderId()) + " -> " + statusToStr(orders[i].getStatus()));
                }
            }
            rowY += 65;
        }
        if (clicked(event, 10, WINDOW_HEIGHT - 80, 100, 30) && kitchenScrollOffset > 0) kitchenScrollOffset--;
        if (clicked(event, 120, WINDOW_HEIGHT - 80, 100, 30) && kitchenScrollOffset < (int)orders.size() - 8) kitchenScrollOffset++;
    }

    void handleFeedbackEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105;
        if (currentRole == UserRole::CUSTOMER && currentCustomer) feedbackCustInput = currentCustomer->getName();
        else {
            if (clicked(event, 15, y + 35, 290, 32)) { typingFbCust = true; typingFbRating = typingFbComment = false; }
            handleTextInput(event, feedbackCustInput, typingFbCust);
        }
        if (clicked(event, 15, y + 100, 290, 32)) { typingFbRating = true;   typingFbCust = typingFbComment = false; }
        if (clicked(event, 15, y + 165, 290, 32)) { typingFbComment = true;  typingFbCust = typingFbRating = false; }
        handleTextInput(event, feedbackRatingInput, typingFbRating);
        handleTextInput(event, feedbackCommentInput, typingFbComment);
        if (clicked(event, 80, y + 220, 160, 38)) {
            string name = (currentRole == UserRole::CUSTOMER && currentCustomer) ? currentCustomer->getName() : feedbackCustInput;
            if (!name.empty() && !feedbackRatingInput.empty()) {
                int rating = max(1, min(10, stoi(feedbackRatingInput)));
                feedbacks.push_back(FeedbackRecord(name, rating, feedbackCommentInput));
                for (auto& c : customers)
                    if (c.getName() == name) { c.setFeedback(rating); FileHandler::saveCustomers(customers); break; }
                showStatus("Feedback submitted! Rating: " + intToStr(rating) + "/10");
                feedbackCustInput = feedbackRatingInput = feedbackCommentInput = "";
            }
            else { showStatus("Enter name and rating."); }
        }
    }

    void handleQueueEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105;
        if (clicked(event, 15, y + 35, 290, 32)) { typingQueueName = true;  typingQueueSize = typingQueueTime = false; }
        if (clicked(event, 15, y + 100, 290, 32)) { typingQueueSize = true;  typingQueueName = typingQueueTime = false; }
        if (clicked(event, 15, y + 165, 290, 32)) { typingQueueTime = true;  typingQueueName = typingQueueSize = false; }
        handleTextInput(event, queueNameInput, typingQueueName);
        handleTextInput(event, queueSizeInput, typingQueueSize);
        handleTextInput(event, queueTimeInput, typingQueueTime);
        if (clicked(event, 60, y + 220, 190, 38)) {
            if (!queueNameInput.empty() && !queueSizeInput.empty() && !queueTimeInput.empty()) {
                waitQueue.push_back(WaitingEntry(queueNameInput, stoi(queueSizeInput), queueTimeInput));
                showStatus(queueNameInput + " added to queue.");
                queueNameInput = queueSizeInput = queueTimeInput = "";
            }
            else { showStatus("Fill all queue fields."); }
        }
        float rowY = y + 30;
        for (int i = 0; i < (int)waitQueue.size(); i++) {
            if (clicked(event, WINDOW_WIDTH - 90, rowY + 10, 60, 28)) {
                showStatus(waitQueue[i].name + " seated!");
                waitQueue.erase(waitQueue.begin() + i);
                break;
            }
            rowY += 57;
            if (rowY > y + 510) break;
        }
    }

    // ── Route events and drawing ──────────────────────────────────────────
    void handleEvents(Event& event) {
        switch (currentScreen) {
        case Screen::LOGIN:       handleLoginEvents(event);       break;
        case Screen::HOME:        handleHomeEvents(event);        break;
        case Screen::MENU:        handleMenuEvents(event);        break;
        case Screen::ORDER:       handleOrderEvents(event);       break;
        case Screen::BILLING:     handleBillingEvents(event);     break;
        case Screen::RESERVATION: handleReservationEvents(event); break;
        case Screen::CUSTOMERS:   handleCustomerEvents(event);    break;
        case Screen::REPORTS:     handleReportEvents(event);      break;
        case Screen::KITCHEN:     handleKitchenEvents(event);     break;
        case Screen::FEEDBACK:    handleFeedbackEvents(event);    break;
        case Screen::QUEUE:       handleQueueEvents(event);       break;
        }
    }

    void drawCurrentScreen() {
        switch (currentScreen) {
        case Screen::LOGIN:       drawLoginScreen();       break;
        case Screen::HOME:        drawHomeScreen();        break;
        case Screen::MENU:        drawMenuScreen();        break;
        case Screen::ORDER:       drawOrderScreen();       break;
        case Screen::BILLING:     drawBillingScreen();     break;
        case Screen::RESERVATION: drawReservationScreen(); break;
        case Screen::CUSTOMERS:   drawCustomerScreen();    break;
        case Screen::REPORTS:     drawReportsScreen();     break;
        case Screen::KITCHEN:     drawKitchenScreen();     break;
        case Screen::FEEDBACK:    drawFeedbackScreen();    break;
        case Screen::QUEUE:       drawQueueScreen();       break;
        }
    }

public:
    RestaurantApp()
        : window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bundu Khan Restaurant - Management System"),
        currentScreen(Screen::LOGIN), currentRole(UserRole::NONE), loggedIn(false),
        currentStaff(nullptr), currentCustomer(nullptr),
        nextMenuId(1), nextOrderId(1), nextCustomerId(1),
        typingUsername(false), typingPassword(false), loginAsCustomer(false),
        typingRegName(false), typingRegPhone(false), typingRegUser(false), typingRegPass(false),
        showRegisterForm(false), menuScrollOffset(0),
        typingMenuName(false), typingMenuPrice(false), typingMenuCat(false),
        selectedMenuItemForEdit(-1), typingMenuEditPrice(false),
        selectedTableForOrder(1), orderMenuScroll(0),
        typingOrderCustomer(false), activeOrder(nullptr),
        orderQtyStr("1"), typingOrderQty(false), selectedMenuItemIndex(0),
        selectedOrderForBill(0),
        reserveNameInput(""), reserveTimeInput(""), selectedTableForRes(1),
        typingResName(false), typingResTime(false),
        typingCustName(false), typingCustPhone(false), custScrollOffset(0),
        kitchenScrollOffset(0),
        typingFbCust(false), typingFbRating(false), typingFbComment(false),
        typingQueueName(false), typingQueueSize(false), typingQueueTime(false)
    {
        window.setFramerateLimit(60);
        gameView.setSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        gameView.setCenter(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f);
        window.setView(gameView);

        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
            if (!font.loadFromFile("/usr/share/fonts/TTF/DejaVuSans.ttf"))
                font.loadFromFile("arial.ttf");

        FileHandler::ensureDataFolder();
        menu = FileHandler::loadMenu();
        customers = FileHandler::loadCustomers();

        // Populate tables
        int caps[] = { 2,2,4,4,4,6,6,8,8,10 };
        for (int i = 0; i < TOTAL_TABLES; i++) tables.push_back(Table(i + 1, caps[i]));

        // Hard-coded staff
        staff.push_back(Staff(1, "Manager Ali", "Manager", "admin", "admin123", 80000));
        staff.push_back(Staff(2, "Waiter Bilal", "Waiter", "waiter1", "pass123", 35000));
        staff.push_back(Staff(3, "Chef Kamran", "Chef", "chef1", "chef123", 50000));

        if (!menu.empty())      nextMenuId = menu.back().getItemId() + 1;
        if (!customers.empty()) nextCustomerId = customers.back().getId() + 1;
        if (menu.empty()) addSampleMenu();
    }

    ~RestaurantApp() { if (activeOrder) { delete activeOrder; activeOrder = nullptr; } }

    void run() {
        while (window.isOpen()) {
            Event event;
            while (window.pollEvent(event)) {
                if (event.type == Event::Closed) window.close();
                if (event.type == Event::Resized) {
                    gameView.setSize(WINDOW_WIDTH, WINDOW_HEIGHT);
                    gameView.setCenter(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f);
                    window.setView(gameView);
                }
                handleEvents(event);
            }
            window.clear(Color(245, 240, 230));
            window.setView(gameView);
            drawCurrentScreen();
            drawStatusBar();
            window.display();
        }
    }
};