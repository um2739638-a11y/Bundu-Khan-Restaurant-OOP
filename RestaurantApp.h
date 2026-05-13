// RestaurantApp: Main application class.
// Handles all screen rendering, user input, and business logic.

#include "DataModels.h"
#include "UIHelper.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
using namespace sf;
using namespace std;

class RestaurantApp {
private:
    // Core SFML objects
    RenderWindow window;
    Font         font;
    Clock        clock;
    View         gameView;

    // Tracks which screen is active and who is logged in
    int  currentScreen;
    int  currentRole;
    bool loggedIn;
    int  currentStaffIdx;
    int  currentCustomerIdx;

    // Status bar message shown at the bottom of the screen (auto-clears after 3.5s)
    string statusMessage;
    Clock  statusClock;

    // Application data arrays
    MenuItem       menu[MAX_MENU];
    int            menuCount;
    Customer       customers[MAX_CUSTOMERS];
    string         customerPasswords[MAX_CUSTOMERS];
    int            customerCount;
    Order          orders[MAX_ORDERS];
    int            orderCount;
    Staff          staff[MAX_STAFF];
    int            staffCount;
    Table          tables[TOTAL_TABLES];
    FeedbackRecord feedbacks[MAX_FEEDBACKS];
    int            feedbackCount;
    WaitingEntry   waitQueue[MAX_QUEUE];
    int            queueCount;

    // Auto-incrementing IDs for new records
    int nextMenuId, nextOrderId, nextCustomerId;

    // Login screen input state
    string loginUsernameInput, loginPasswordInput;
    bool   typingUsername, typingPassword, loginAsCustomer;

    // Registration form input state
    string regNameInput, regPhoneInput, regUserInput, regPassInput;
    bool   typingRegName, typingRegPhone, typingRegUser, typingRegPass;
    bool   showRegisterForm;

    // Menu screen input state
    int    menuScrollOffset;
    string menuNameInput, menuPriceInput, menuCatInput;
    bool   typingMenuName, typingMenuPrice, typingMenuCat;
    int    selectedMenuItemForEdit;
    string menuEditPriceInput;
    bool   typingMenuEditPrice;

    // Order screen input state
    int    selectedTableForOrder;
    int    orderMenuScroll;
    string orderCustomerInput;
    bool   typingOrderCustomer;
    int    activeOrderIdx;
    string orderQtyStr;
    bool   typingOrderQty;
    int    selectedMenuItemIndex;

    int selectedOrderForBill;

    // Reservation screen input state
    string reserveNameInput, reserveTimeInput;
    int    selectedTableForRes;
    bool   typingResName, typingResTime;

    // Customer screen input state
    string custNameInput, custPhoneInput;
    bool   typingCustName, typingCustPhone;
    int    custScrollOffset;

    int kitchenScrollOffset;

    // Feedback screen input state
    string feedbackCustInput, feedbackRatingInput, feedbackCommentInput;
    bool   typingFbCust, typingFbRating, typingFbComment;

    // Queue screen input state
    string queueNameInput, queueSizeInput, queueTimeInput;
    bool   typingQueueName, typingQueueSize, typingQueueTime;

    // Cached integrity check results shown on the reports screen
    float verifiedRevenue;
    float rawRevenue;
    int   corruptRecords;
    int   missingRecords;
    bool  integrityChecked;

    //  Private helpers 

    // Shorthand for UIHelper::isClicked — checks if a mouse click hit a rect
    bool clicked(Event& e, float x, float y, float w, float h) {
        return UIHelper::isClicked(e, window, x, y, w, h);
    }

    // Sets the status bar message and resets its display timer
    void showStatus(const string& msg) {
        statusMessage = msg; statusClock.restart();
    }

    // Appends a typed character to str if the field is active; handles backspace
    void handleTextInput(Event& event, string& str, bool isActive) {
        if (!isActive || event.type != Event::TextEntered) return;
        if (event.text.unicode == 8 && !str.empty()) str.pop_back();
        else if (event.text.unicode >= 32 && event.text.unicode < 128)
            str += (char)event.text.unicode;
    }

    // Same as handleTextInput but only allows digits and caps input at 11 chars
    void handlePhoneInput(Event& event, string& str, bool isActive) {
        if (!isActive || event.type != Event::TextEntered) return;
        if (event.text.unicode == 8 && !str.empty()) { str.pop_back(); return; }
        char c = (char)event.text.unicode;
        if (isdigit(c) && (int)str.size() < 11) str += c;
    }

    // Allows digits and one decimal point; used for price and quantity fields
    void handleNumericInput(Event& event, string& str, bool isActive, int maxLen = 8) {
        if (!isActive || event.type != Event::TextEntered) return;
        if (event.text.unicode == 8 && !str.empty()) { str.pop_back(); return; }
        char c = (char)event.text.unicode;
        bool hasDot = (str.find('.') != string::npos);
        if ((isdigit(c) || (c == '.' && !hasDot)) && (int)str.size() < maxLen)
            str += c;
    }

    // Draws a labeled text input box; masked=true shows asterisks (for passwords)
    void drawInputBox(float x, float y, float w, float h,
        const string& label, const string& value,
        bool active, bool masked = false)
    {
        Color bg = active ? Color(255, 255, 230) : Color(240, 240, 240);
        UIHelper::drawRect(window, x, y, w, h, bg, Color(100, 100, 100), 1.5f);
        UIHelper::drawText(window, font, label, x, y - 18, 13, Color(60, 60, 60));
        string dv = masked ? string(value.size(), '*') : value;
        dv += active ? "|" : "";
        UIHelper::drawText(window, font, dv, x + 5, y + 8, 15, Color(20, 20, 20));
    }

    // Returns a pointer to the currently logged-in customer, or nullptr if invalid
    Customer* currentCustomer() {
        if (currentCustomerIdx < 0 || currentCustomerIdx >= customerCount) return nullptr;
        return &customers[currentCustomerIdx];
    }

    // Returns a pointer to the currently logged-in staff member, or nullptr if invalid
    Staff* currentStaff() {
        if (currentStaffIdx < 0 || currentStaffIdx >= staffCount) return nullptr;
        return &staff[currentStaffIdx];
    }

    // Returns true if a menu item with the same name already exists (case-insensitive).
    // skipIdx lets you ignore the item being edited so it doesn't conflict with itself.
    bool menuNameExists(const string& name, int skipIdx = -1) {
        string lower = name;
        for (char& c : lower) c = tolower(c);
        for (int i = 0; i < menuCount; i++) {
            if (i == skipIdx) continue;
            string existing = menu[i].getItemName();
            for (char& c : existing) c = tolower(c);
            if (existing == lower) return true;
        }
        return false;
    }

    // Returns true if a party with the same name is already in the waiting queue
    bool queueNameExists(const string& name) {
        string lower = name;
        for (char& c : lower) c = tolower(c);
        for (int i = 0; i < queueCount; i++) {
            string qn = waitQueue[i].getName();
            for (char& c : qn) c = tolower(c);
            if (qn == lower) return true;
        }
        return false;
    }

    // Returns true if the table is reserved by someone other than the given customer
    bool tableReservedBySomeoneElse(int tableIdx, const string& custName) {
        if (!tables[tableIdx].isReserved()) return false;
        return tables[tableIdx].getReservedFor() != custName;
    }

    // Records the visit and total spent for a customer after their order is paid
    void updateCustomerAfterPayment(const Order& ord) {
        for (int i = 0; i < customerCount; i++) {
            if (customers[i].getName() == ord.getCustomerName()) {
                customers[i].recordVisit(ord.getTotal());
                FileHandler::saveCustomersFull(customers, customerCount, customerPasswords);
                break;
            }
        }
    }

    // Prevents scroll offsets from going out of bounds on all scrollable lists
    void clampScrollOffsets() {
        if (menuScrollOffset < 0) menuScrollOffset = 0;
        if (menuScrollOffset > menuCount - 16 && menuCount > 16)
            menuScrollOffset = menuCount - 16;
        if (menuCount <= 16) menuScrollOffset = 0;

        if (custScrollOffset < 0) custScrollOffset = 0;
        if (custScrollOffset > customerCount - 15 && customerCount > 15)
            custScrollOffset = customerCount - 15;
        if (customerCount <= 15) custScrollOffset = 0;

        if (kitchenScrollOffset < 0) kitchenScrollOffset = 0;
        if (kitchenScrollOffset > orderCount - 8 && orderCount > 8)
            kitchenScrollOffset = orderCount - 8;
        if (orderCount <= 8) kitchenScrollOffset = 0;
    }

    // Seeds the menu with default items the first time the app runs (no saved data)
    void addSampleMenu() {
        auto add = [&](const string& name, float price, const string& cat) {
            if (menuCount < MAX_MENU)
                menu[menuCount++] = MenuItem(nextMenuId++, name, price, cat);
            };
        add("Chicken Karahi", 850, "Main Course");
        add("Mutton Karahi", 1200, "Main Course");
        add("Beef Biryani", 450, "Main Course");
        add("Chicken Tikka", 650, "Starter");
        add("Seekh Kebab", 480, "Starter");
        add("Naan", 60, "Bread");
        add("Tandoori Roti", 40, "Bread");
        add("Raita", 80, "Side");
        add("Lassi Sweet", 150, "Drink");
        add("Cold Drink", 80, "Drink");
        add("Gulab Jamun", 200, "Dessert");
        add("Kheer", 150, "Dessert");
        FileHandler::saveMenu(menu, menuCount);
    }

    // ── Shared UI helpers ─────────────────────────────────────

    // Draws the yellow status message bar at the bottom; clears after 3.5 seconds
    void drawStatusBar() {
        if (statusClock.getElapsedTime().asSeconds() > 3.5f) statusMessage = "";
        if (!statusMessage.empty()) {
            UIHelper::drawRect(window, 0, WINDOW_HEIGHT - 35, WINDOW_WIDTH, 35, Color(30, 30, 30, 220));
            UIHelper::drawText(window, font, statusMessage, 10, WINDOW_HEIGHT - 28, 16, Color::Yellow);
        }
    }

    // Draws the red top bar with the restaurant name, screen title, and logged-in user
    void drawTopBar(const string& screenTitle) {
        UIHelper::drawRect(window, 0, 0, WINDOW_WIDTH, 60, Color(120, 20, 20));
        UIHelper::drawText(window, font, "Bundu Khan Restaurant", 15, 15, 22, Color(255, 220, 100));
        UIHelper::drawText(window, font, ">> " + screenTitle + " <<", 350, 18, 18, Color::White);
        Staff* s = currentStaff();
        Customer* c = currentCustomer();
        if (currentRole == ROLE_STAFF && s)
            UIHelper::drawText(window, font, "Admin: " + s->getName() + " (" + s->getRole() + ")", 720, 10, 13, Color(200, 255, 200));
        else if (currentRole == ROLE_CUSTOMER && c) {
            UIHelper::drawText(window, font, "Customer: " + c->getName(), 750, 10, 13, Color(200, 220, 255));
            UIHelper::drawText(window, font, "Visits: " + intToStr(c->getVisits()), 750, 28, 12, Color(180, 200, 255));
        }
    }

    // Draws the navigation tab buttons below the top bar.
    // Staff see fewer tabs than customers.
    void drawNavBar() {
        float y = 65, h = 32, x = 5;
        if (currentRole == ROLE_STAFF) {
            float w = 87;
            const char* labels[] = { "Home","Billing","Reserve","Queue" };
            int scrs[] = { SCR_HOME,SCR_BILLING,SCR_RESERVATION,SCR_QUEUE };
            for (int i = 0; i < 4; i++) {
                Color c = (currentScreen == scrs[i]) ? Color(200, 150, 50) : Color(60, 60, 80);
                UIHelper::drawButton(window, font, labels[i], x + i * (w + 3), y, w, h, c, Color::White, 13);
            }
        }
        else {
            float w = 135;
            const char* labels[] = { "Home","View Menu","My Order","My Bill","Reserve","Feedback" };
            int scrs[] = { SCR_HOME,SCR_MENU,SCR_ORDER,SCR_BILLING,SCR_RESERVATION,SCR_FEEDBACK };
            for (int i = 0; i < 6; i++) {
                Color c = (currentScreen == scrs[i]) ? Color(20, 100, 200) : Color(40, 60, 100);
                UIHelper::drawButton(window, font, labels[i], x + i * (w + 3), y, w, h, c, Color::White, 13);
            }
        }
    }

    // Switches currentScreen when a nav tab is clicked
    void handleNavBarClick(Event& event) {
        float y = 65, h = 32, x = 5;
        if (currentRole == ROLE_STAFF) {
            float w = 87;
            int scrs[] = { SCR_HOME,SCR_BILLING,SCR_RESERVATION,SCR_QUEUE };
            for (int i = 0; i < 4; i++)
                if (clicked(event, x + i * (w + 3), y, w, h)) currentScreen = scrs[i];
        }
        else {
            float w = 135;
            int scrs[] = { SCR_HOME,SCR_MENU,SCR_ORDER,SCR_BILLING,SCR_RESERVATION,SCR_FEEDBACK };
            for (int i = 0; i < 6; i++)
                if (clicked(event, x + i * (w + 3), y, w, h)) currentScreen = scrs[i];
        }
    }

    // ── Screen draw methods ───────────────────────────────────

    // Draws the login/registration screen.
    // Switches between login form and registration form via showRegisterForm flag.
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
            UIHelper::drawButton(window, font, "Admin", cx + 120, cy, 100, 30, sc, Color::White, 15);
            UIHelper::drawButton(window, font, "Customer", cx + 228, cy, 120, 30, cc, Color::White, 15);
            string title = loginAsCustomer ? "Customer Login" : "Admin Login";
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
                UIHelper::drawText(window, font, "Enter your ID to access Admin panel", cx + 70, cy + 285, 13, Color(100, 100, 100));
            }
        }
        else {
            UIHelper::drawRect(window, cx - 10, cy - 10, 430, 430, Color(255, 255, 255, 220), Color(0, 100, 60), 2);
            UIHelper::drawText(window, font, "New Customer Registration", cx + 50, cy + 2, 18, Color(0, 80, 50));
            drawInputBox(cx + 10, cy + 38, 390, 34, "Full Name:", regNameInput, typingRegName);
            drawInputBox(cx + 10, cy + 108, 390, 34, "Phone (11 digits):", regPhoneInput, typingRegPhone);
            drawInputBox(cx + 10, cy + 178, 390, 34, "Username:", regUserInput, typingRegUser);
            drawInputBox(cx + 10, cy + 248, 390, 34, "Password (min 8 chars):", regPassInput, typingRegPass, true);
            UIHelper::drawButton(window, font, "Create Account", cx + 100, cy + 308, 200, 40, Color(0, 120, 60), Color::White, 16);
            UIHelper::drawButton(window, font, "< Back to Login", cx + 100, cy + 360, 200, 32, Color(100, 100, 100), Color::White, 14);
        }
    }

    // Draws the dashboard with summary cards and quick-action buttons
    void drawHomeScreen() {
        drawTopBar("Dashboard");
        drawNavBar();
        float startY = 110;
        UIHelper::drawRect(window, 10, startY, WINDOW_WIDTH - 20, 60, Color(80, 30, 10));
        Customer* c = currentCustomer();
        string welcome = (currentRole == ROLE_CUSTOMER && c)
            ? "Welcome, " + c->getName() + "! | Visits: " + intToStr(c->getVisits())
            : "Welcome to Bundu Khan Restaurant Management System";
        UIHelper::drawText(window, font, welcome, 25, startY + 18, 18, Color(255, 220, 100));

        float cardY = startY + 80, cardW = 200, cardH = 100, gap = 20;
        auto drawCard = [&](float x, Color col, const string& lbl, const string& val) {
            UIHelper::drawRect(window, x, cardY, cardW, cardH, col, Color::White, 1);
            UIHelper::drawText(window, font, lbl, x + 10, cardY + 10, 15, Color::White);
            UIHelper::drawText(window, font, val, x + 65, cardY + 40, 32, Color::Yellow);
            };
        drawCard(10, Color(40, 100, 160), "Active Orders", intToStr(orderCount));
        drawCard(10 + cardW + gap, Color(40, 130, 60), "Total Tables", intToStr(TOTAL_TABLES));
        int res = 0; for (int i = 0; i < TOTAL_TABLES; i++) if (tables[i].isReserved()) res++;
        drawCard(10 + 2 * (cardW + gap), Color(160, 80, 0), "Reserved", intToStr(res));
        drawCard(10 + 3 * (cardW + gap), Color(120, 30, 120), "Customers", intToStr(customerCount));
        drawCard(10 + 4 * (cardW + gap), Color(30, 80, 130), "Menu Items", intToStr(menuCount));

        float raw = 0; int corrupt = 0, missing = 0;
        float verified = FileHandler::loadTodaysSalesVerified(raw, corrupt, missing);
        UIHelper::drawRect(window, 10, cardY + cardH + 20, WINDOW_WIDTH - 20, 65, Color(50, 50, 50));
        UIHelper::drawText(window, font, "Today's Verified Revenue: Rs." + floatToStr(verified), 20, cardY + cardH + 28, 20, Color(100, 255, 100));

        float btnY = cardY + cardH + 105;
        UIHelper::drawText(window, font, "Quick Actions:", 10, btnY, 18, Color(50, 50, 50));
        btnY += 28;
        if (currentRole == ROLE_STAFF) {
            UIHelper::drawButton(window, font, "Generate Bill", 10, btnY, 160, 40, Color(160, 80, 0), Color::White);
            UIHelper::drawButton(window, font, "Reserve", 180, btnY, 160, 40, Color(20, 100, 180), Color::White);
            UIHelper::drawButton(window, font, "Queue", 350, btnY, 160, 40, Color(120, 50, 0), Color::White);
        }
        else {
            UIHelper::drawButton(window, font, "View Menu", 10, btnY, 160, 40, Color(40, 130, 60), Color::White);
            UIHelper::drawButton(window, font, "Place Order", 180, btnY, 160, 40, Color(40, 100, 160), Color::White);
            UIHelper::drawButton(window, font, "My Bill", 350, btnY, 160, 40, Color(160, 80, 0), Color::White);
            UIHelper::drawButton(window, font, "Reserve Table", 520, btnY, 160, 40, Color(20, 100, 180), Color::White);
        }
        UIHelper::drawButton(window, font, "LOGOUT", WINDOW_WIDTH - 130, WINDOW_HEIGHT - 50, 120, 35, Color(180, 20, 20), Color::White);
        if (queueCount > 0)
            UIHelper::drawText(window, font, "Waiting Queue: " + intToStr(queueCount) + " group(s) waiting", 10, btnY + 60, 16, Color(180, 60, 0));
    }

    // Draws the menu list; staff can add, edit price, and delete items
    void drawMenuScreen() {
        drawTopBar("Menu Management");
        drawNavBar();
        float panelY = 105;
        UIHelper::drawRect(window, 5, panelY, 310, 300, Color(245, 245, 255), Color(100, 100, 200), 1);
        if (currentRole == ROLE_STAFF) {
            UIHelper::drawText(window, font, "Add New Item", 15, panelY + 5, 16, Color(30, 30, 100));
            drawInputBox(15, panelY + 35, 290, 32, "Item Name (letters/spaces):", menuNameInput, typingMenuName);
            drawInputBox(15, panelY + 100, 290, 32, "Price Rs (1-99999):", menuPriceInput, typingMenuPrice);
            drawInputBox(15, panelY + 165, 290, 32, "Category (letters/spaces):", menuCatInput, typingMenuCat);
            UIHelper::drawButton(window, font, "Add Item", 80, panelY + 210, 160, 35, Color(40, 130, 60), Color::White);
            UIHelper::drawRect(window, 5, panelY + 250, 310, 115, Color(255, 250, 235), Color(180, 120, 0), 1);
            UIHelper::drawText(window, font, "Update Item Price", 15, panelY + 255, 14, Color(100, 60, 0));
            if (selectedMenuItemForEdit >= 0 && selectedMenuItemForEdit < menuCount) {
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
            UIHelper::drawText(window, font, "(Use 'My Order' to place an order)", 30, panelY + 155, 12, Color(100, 100, 150));
        }
        UIHelper::drawRect(window, 325, panelY, WINDOW_WIDTH - 335, 530, Color(250, 250, 250), Color(150, 150, 150), 1);
        UIHelper::drawText(window, font, "Current Menu (" + intToStr(menuCount) + " items):", 335, panelY + 5, 15, Color(30, 30, 80));
        int visible = 16; float itemY = panelY + 28;
        for (int i = menuScrollOffset; i < menuCount && i < menuScrollOffset + visible; i++) {
            float rowY = itemY + (i - menuScrollOffset) * 30;
            Color rowBg = (selectedMenuItemForEdit == i) ? Color(255, 240, 180)
                : (i % 2 == 0) ? Color(240, 240, 255) : Color(255, 255, 255);
            UIHelper::drawRect(window, 326, rowY, WINDOW_WIDTH - 337, 28, rowBg);
            UIHelper::drawText(window, font, menu[i].getDisplayLine(), 330, rowY + 5, 13, Color(20, 20, 20));
            if (currentRole == ROLE_STAFF)
                UIHelper::drawButton(window, font, "Del", WINDOW_WIDTH - 60, rowY + 2, 40, 24, Color(180, 30, 30), Color::White, 12);
        }
        UIHelper::drawButton(window, font, "^ Up", WINDOW_WIDTH - 80, panelY + 465, 75, 28, Color(80, 80, 80), Color::White, 13);
        UIHelper::drawButton(window, font, "v Down", WINDOW_WIDTH - 80, panelY + 498, 75, 28, Color(80, 80, 80), Color::White, 13);
    }

    // Draws the order screen: table selection, menu browsing, and active order summary
    void drawOrderScreen() {
        drawTopBar("Order Management"); drawNavBar();
        float y = 105; Customer* c = currentCustomer();
        UIHelper::drawRect(window, 5, y, 320, 530, Color(245, 252, 245), Color(0, 120, 0), 1);
        UIHelper::drawText(window, font, "New Order Setup", 15, y + 5, 16, Color(0, 80, 0));
        string custDisplay = (currentRole == ROLE_CUSTOMER && c) ? c->getName() : orderCustomerInput;
        drawInputBox(15, y + 35, 300, 32, "Customer Name:", custDisplay, typingOrderCustomer);
        UIHelper::drawText(window, font, "Select Table (Green=free, Red=reserved):", 15, y + 80, 12, Color(50, 50, 50));
        for (int i = 0; i < TOTAL_TABLES; i++) {
            float tx = 15 + (i % 5) * 58, ty = y + 100 + (i / 5) * 38;
            bool  res = tables[i].isReserved();
            Color tc = (selectedTableForOrder == i + 1) ? Color(0, 180, 0)
                : res ? Color(180, 50, 50) : Color(100, 150, 100);
            UIHelper::drawButton(window, font, "T" + intToStr(i + 1), tx, ty, 50, 30, tc, Color::White, 13);
        }
        UIHelper::drawText(window, font, "Select Item:", 15, y + 182, 14, Color(50, 50, 50));
        for (int i = orderMenuScroll; i < menuCount && i < orderMenuScroll + 8; i++) {
            float ry = y + 200 + (i - orderMenuScroll) * 26;
            Color rc = (selectedMenuItemIndex == i) ? Color(0, 180, 0, 80) : Color::Transparent;
            UIHelper::drawRect(window, 15, ry, 295, 24, rc);
            UIHelper::drawText(window, font, menu[i].getItemName() + " Rs." + floatToStr(menu[i].getPrice()), 17, ry + 4, 13, Color(10, 10, 10));
        }
        UIHelper::drawText(window, font, "Qty (1-50):", 15, y + 412, 13, Color(50, 50, 50));
        drawInputBox(90, y + 408, 60, 28, "", orderQtyStr, typingOrderQty);
        UIHelper::drawButton(window, font, "Add to Order", 160, y + 408, 140, 28, Color(0, 120, 0), Color::White, 13);
        UIHelper::drawButton(window, font, "CONFIRM ORDER", 20, y + 450, 280, 40, Color(20, 80, 20), Color::White, 16);
        UIHelper::drawButton(window, font, "^", 290, y + 200, 25, 25, Color(80, 80, 80), Color::White, 14);
        UIHelper::drawButton(window, font, "v", 290, y + 410, 25, 25, Color(80, 80, 80), Color::White, 14);

        UIHelper::drawRect(window, 335, y, WINDOW_WIDTH - 345, 530, Color(252, 252, 245), Color(150, 150, 0), 1);
        string orderHeader = "Current Order";
        if (activeOrderIdx >= 0 && activeOrderIdx < orderCount)
            orderHeader += " (Table " + intToStr(orders[activeOrderIdx].getTableNumber()) + ")";
        else orderHeader += ": None";
        UIHelper::drawText(window, font, orderHeader, 345, y + 5, 16, Color(80, 60, 0));
        if (activeOrderIdx >= 0 && activeOrderIdx < orderCount) {
            Order& ao = orders[activeOrderIdx];
            UIHelper::drawText(window, font, "Customer: " + ao.getCustomerName(), 345, y + 30, 14, Color(40, 40, 40));
            float rowY = y + 55;
            for (int i = 0; i < ao.getItemCount(); i++) {
                const OrderItem& oi = ao.getItem(i);
                UIHelper::drawText(window, font,
                    oi.getItem().getItemName() + " x" + intToStr(oi.getQuantity()) + " = Rs." + floatToStr(oi.getSubtotal()),
                    345, rowY, 14, Color(20, 20, 80));
                UIHelper::drawButton(window, font, "X", WINDOW_WIDTH - 55, rowY - 3, 30, 22, Color(180, 30, 30), Color::White, 12);
                rowY += 28;
            }
            UIHelper::drawRect(window, 335, y + 400, WINDOW_WIDTH - 345, 130, Color(240, 245, 220));
            UIHelper::drawText(window, font, "Subtotal:    Rs." + floatToStr(ao.getSubtotal()), 345, y + 410, 15, Color(30, 30, 30));
            UIHelper::drawText(window, font, "Tax(17%):    Rs." + floatToStr(ao.getTax()), 345, y + 430, 15, Color(30, 30, 30));
            UIHelper::drawText(window, font, "Service(5%): Rs." + floatToStr(ao.getServiceCharge()), 345, y + 450, 15, Color(30, 30, 30));
            UIHelper::drawText(window, font, "TOTAL:       Rs." + floatToStr(ao.getTotal()), 345, y + 475, 18, Color(180, 0, 0));
        }
    }

    // Staff see YES/NO confirmation; customers see cash/card payment options
    void drawBillingScreen() {
        drawTopBar("Billing"); drawNavBar();
        float y = 105; Customer* c = currentCustomer();
        if (currentRole == ROLE_STAFF) {
            UIHelper::drawText(window, font, "Pending Bills:", 20, y, 18, Color(20, 20, 80));
            int unpaid = 0;
            for (int i = 0; i < orderCount; i++) {
                if (!orders[i].isPaid()) {
                    float ry = y + 40 + unpaid * 45;
                    Color bg = (selectedOrderForBill == i) ? Color(255, 230, 150) : Color(240, 240, 240);
                    UIHelper::drawRect(window, 10, ry, 500, 38, bg, Color(150, 150, 0), 1);
                    UIHelper::drawText(window, font, "Order #" + intToStr(orders[i].getOrderId()) + " | " + orders[i].getCustomerName() + " | Rs." + floatToStr(orders[i].getTotal()), 18, ry + 10, 14, Color(20, 20, 20));
                    UIHelper::drawButton(window, font, "Select", 520, ry + 6, 70, 26, Color(80, 80, 20), Color::White, 12);
                    unpaid++;
                }
            }
            if (unpaid == 0) { UIHelper::drawText(window, font, "No pending bills.", 380, y + 50, 20, Color(100, 100, 100)); return; }
            if (selectedOrderForBill >= 0 && selectedOrderForBill < orderCount && !orders[selectedOrderForBill].isPaid()) {
                UIHelper::drawText(window, font, "Bill Paid?", WINDOW_WIDTH / 2 - 60, y + 280, 28, Color(20, 20, 20));
                UIHelper::drawButton(window, font, "YES", WINDOW_WIDTH / 2 - 150, y + 330, 120, 60, Color(0, 150, 0), Color::White, 28);
                UIHelper::drawButton(window, font, "NO", WINDOW_WIDTH / 2 + 30, y + 330, 120, 60, Color(180, 30, 30), Color::White, 28);
            }
            return;
        }
        int unpaid = 0;
        for (int i = 0; i < orderCount; i++) {
            if (!orders[i].isPaid() && c && orders[i].getCustomerName() == c->getName()) {
                float ry = y + 30 + unpaid * 50;
                Color bg = (selectedOrderForBill == i) ? Color(255, 230, 150) : Color(248, 245, 235);
                UIHelper::drawRect(window, 8, ry, 500, 40, bg, Color(180, 140, 0), 1);
                UIHelper::drawText(window, font, "Order #" + intToStr(orders[i].getOrderId()) + " | Rs." + floatToStr(orders[i].getTotal()), 15, ry + 10, 14, Color(30, 30, 30));
                UIHelper::drawButton(window, font, "Select", 520, ry + 7, 70, 26, Color(80, 80, 20), Color::White, 12);
                unpaid++;
            }
        }
        if (unpaid == 0) { UIHelper::drawText(window, font, "No pending bills.", 380, y + 40, 18, Color(100, 100, 100)); return; }
        if (selectedOrderForBill >= 0 && selectedOrderForBill < orderCount) {
            Order& ord = orders[selectedOrderForBill];
            float by = y + 300;
            if (ord.isAwaitingCashConfirm())
                UIHelper::drawText(window, font, "Awaiting staff confirmation...", 20, by, 16, Color(100, 60, 0));
            else {
                UIHelper::drawButton(window, font, "Pay CASH", 20, by, 160, 40, Color(0, 130, 0), Color::White, 16);
                UIHelper::drawButton(window, font, "Pay CARD", 200, by, 160, 40, Color(0, 80, 160), Color::White, 16);
            }
        }
    }

    // Customers can reserve a table by time slot; staff can cancel any reservation
    void drawReservationScreen() {
        drawTopBar("Table Reservation"); drawNavBar();
        float y = 105;
        if (currentRole == ROLE_CUSTOMER) {
            Customer* c = currentCustomer();
            drawInputBox(15, y + 20, 290, 32, "Time (e.g. 7PM):", reserveTimeInput, typingResTime);
            UIHelper::drawText(window, font, "Select Table:", 15, y + 65, 14, Color(50, 50, 50));
            for (int i = 0; i < TOTAL_TABLES; i++) {
                float tx = 15 + (i % 5) * 58, ty = y + 85 + (i / 5) * 38;
                Color tc = tables[i].isReserved() ? Color(180, 50, 50)
                    : (selectedTableForRes == i + 1) ? Color(0, 120, 200) : Color(80, 80, 180);
                UIHelper::drawButton(window, font, "T" + intToStr(i + 1), tx, ty, 50, 30, tc, Color::White, 13);
            }
            UIHelper::drawButton(window, font, "Reserve", 60, y + 165, 160, 35, Color(0, 0, 150), Color::White);
        }
        float rowY = y + 220; bool any = false;
        UIHelper::drawText(window, font, "Current Reservations:", 15, rowY - 20, 15, Color(0, 0, 100));
        for (int i = 0; i < TOTAL_TABLES; i++) {
            if (tables[i].isReserved()) {
                any = true;
                UIHelper::drawText(window, font, "Table " + intToStr(tables[i].getTableNum()) + " - " + tables[i].getReservedFor() + " at " + tables[i].getReservationTime(), 15, rowY, 14, Color(20, 20, 80));
                if (currentRole == ROLE_STAFF)
                    UIHelper::drawButton(window, font, "Cancel", WINDOW_WIDTH - 110, rowY - 3, 80, 25, Color(180, 30, 30), Color::White, 13);
                rowY += 35;
            }
        }
        if (!any) UIHelper::drawText(window, font, "No reservations.", 400, rowY, 16, Color(120, 120, 120));
    }

    // Staff can add walk-in customers; customers see their own profile summary
    void drawCustomerScreen() {
        drawTopBar("Customer Management"); drawNavBar();
        float y = 105; Customer* c = currentCustomer();
        if (currentRole == ROLE_STAFF) {
            UIHelper::drawRect(window, 5, y, 310, 200, Color(252, 245, 255), Color(120, 0, 120), 1);
            UIHelper::drawText(window, font, "Add New Customer", 15, y + 5, 16, Color(80, 0, 80));
            drawInputBox(15, y + 35, 290, 32, "Customer Name:", custNameInput, typingCustName);
            drawInputBox(15, y + 100, 290, 32, "Phone (11 digits):", custPhoneInput, typingCustPhone);
            UIHelper::drawButton(window, font, "Add Customer", 60, y + 155, 190, 35, Color(100, 0, 100), Color::White);
        }
        else {
            UIHelper::drawRect(window, 5, y, 310, 130, Color(230, 240, 255), Color(0, 60, 140), 1);
            UIHelper::drawText(window, font, "Your Profile", 80, y + 10, 16, Color(0, 40, 100));
            if (c) {
                UIHelper::drawText(window, font, "Name:        " + c->getName(), 15, y + 35, 14, Color(20, 20, 80));
                UIHelper::drawText(window, font, "Phone:       " + c->getPhone(), 15, y + 55, 14, Color(20, 20, 80));
                UIHelper::drawText(window, font, "Visits:      " + intToStr(c->getVisits()), 15, y + 75, 14, Color(20, 20, 80));
                UIHelper::drawText(window, font, "Total Spent: Rs." + floatToStr(c->getTotalSpent()), 15, y + 95, 14, Color(20, 20, 80));
            }
        }
        UIHelper::drawRect(window, 325, y, WINDOW_WIDTH - 335, 530, Color(252, 248, 255), Color(150, 0, 150), 1);
        UIHelper::drawText(window, font, "Registered Customers (" + intToStr(customerCount) + "):", 335, y + 5, 15, Color(80, 0, 80));
        int visible = 15; float rowY = y + 28;
        for (int i = custScrollOffset; i < customerCount && i < custScrollOffset + visible; i++) {
            Color bg = (i % 2 == 0) ? Color(245, 235, 255) : Color(255, 255, 255);
            UIHelper::drawRect(window, 327, rowY, WINDOW_WIDTH - 338, 32, bg);
            UIHelper::drawText(window, font, customers[i].getSummary(), 332, rowY + 7, 13, Color(20, 20, 20));
            if (customers[i].getFeedback() > 0)
                UIHelper::drawText(window, font, "Rating: " + intToStr(customers[i].getFeedback()) + "/5", WINDOW_WIDTH - 150, rowY + 7, 12, Color(0, 100, 0));
            rowY += 34;
        }
        UIHelper::drawButton(window, font, "^ Up", WINDOW_WIDTH - 80, y + 490, 75, 30, Color(80, 80, 80), Color::White, 13);
        UIHelper::drawButton(window, font, "v Down", WINDOW_WIDTH - 80, y + 525, 75, 30, Color(80, 80, 80), Color::White, 13);
    }

    // Staff-only screen: verified revenue, order list, popular items, and average rating
    void drawReportsScreen() {
        drawTopBar("Daily Reports"); drawNavBar();
        if (currentRole == ROLE_CUSTOMER) {
            UIHelper::drawText(window, font, "Access Denied", 400, 300, 30, Color(180, 0, 0));
            UIHelper::drawText(window, font, "This screen is for staff only.", 320, 350, 18, Color(120, 0, 0)); return;
        }
        float y = 110;
        UIHelper::drawRect(window, 10, y, WINDOW_WIDTH - 20, 510, Color(245, 255, 245), Color(0, 120, 0), 2);
        UIHelper::drawText(window, font, "Daily Sales Report - " + getTodayDate(), 30, y + 10, 20, Color(0, 80, 0));
        float raw = 0; int corrupt = 0, missing = 0;
        float verified = FileHandler::loadTodaysSalesVerified(raw, corrupt, missing);
        int   todayOrders = FileHandler::loadTodaysOrderCount();
        UIHelper::drawRect(window, 20, y + 45, 300, 80, Color(0, 120, 0, 30));
        UIHelper::drawText(window, font, "Verified Revenue Today:", 30, y + 55, 15, Color(20, 20, 20));
        UIHelper::drawText(window, font, "Rs. " + floatToStr(verified), 30, y + 78, 22, Color(0, 130, 0));
        UIHelper::drawRect(window, 340, y + 45, 300, 80, Color(0, 0, 150, 30));
        UIHelper::drawText(window, font, "Total Orders Today:", 350, y + 55, 15, Color(20, 20, 20));
        UIHelper::drawText(window, font, intToStr(todayOrders), 350, y + 78, 22, Color(0, 0, 150));
        UIHelper::drawRect(window, 660, y + 45, 300, 80, Color(150, 0, 0, 30));
        UIHelper::drawText(window, font, "Total Customers:", 670, y + 55, 15, Color(20, 20, 20));
        UIHelper::drawText(window, font, intToStr(customerCount), 670, y + 78, 22, Color(150, 0, 0));
        UIHelper::drawText(window, font, "Active Orders Details:", 30, y + 168, 16, Color(30, 30, 80));
        float rowY = y + 190;
        if (orderCount > 0) {
            for (int i = 0; i < orderCount; i++) {
                UIHelper::drawText(window, font,
                    "Order #" + intToStr(orders[i].getOrderId()) + " | Table " + intToStr(orders[i].getTableNumber()) +
                    " | " + orders[i].getCustomerName() + " | Rs." + floatToStr(orders[i].getTotal()) +
                    " | " + statusToStr(orders[i].getStatus()) + (orders[i].isPaid() ? " [PAID]" : " [UNPAID]"),
                    30, rowY, 13, Color(20, 20, 60));
                rowY += 22; if (rowY > y + 360) break;
            }
        }
        else {
            UIHelper::drawText(window, font, "No active orders.", 30, rowY, 15, Color(100, 100, 100));
        }
        // Count total quantity ordered per item across all active orders
        string itemNames[MAX_MENU]; int itemTotals[MAX_MENU]; int itemFreqCount = 0;
        for (int o = 0; o < orderCount; o++)
            for (int k = 0; k < orders[o].getItemCount(); k++) {
                const string& nm = orders[o].getItem(k).getItem().getItemName();
                int           qty = orders[o].getItem(k).getQuantity();
                bool found = false;
                for (int f = 0; f < itemFreqCount; f++)
                    if (itemNames[f] == nm) { itemTotals[f] += qty; found = true; break; }
                if (!found && itemFreqCount < MAX_MENU) { itemNames[itemFreqCount] = nm; itemTotals[itemFreqCount] = qty; itemFreqCount++; }
            }
        UIHelper::drawText(window, font, "Most Ordered Items (Active):", 30, y + 370, 15, Color(80, 0, 0));
        float miY = y + 392;
        for (int i = 0; i < itemFreqCount; i++) {
            UIHelper::drawText(window, font, itemNames[i] + ": " + intToStr(itemTotals[i]) + " portions", 30, miY, 13, Color(30, 30, 30));
            miY += 20; if (miY > y + 490) break;
        }
        if (feedbackCount > 0) {
            float avg = 0;
            for (int i = 0; i < feedbackCount; i++) avg += feedbacks[i].getRating();
            avg /= feedbackCount;
            UIHelper::drawText(window, font, "Avg Rating: " + floatToStr(avg) + "/5 (" + intToStr(feedbackCount) + " feedbacks)", 600, y + 370, 13, Color(0, 120, 0));
        }
    }

    // Staff-only: shows all active orders with current status and an Advance button
    void drawKitchenScreen() {
        drawTopBar("Kitchen Order Tracking"); drawNavBar();
        if (currentRole == ROLE_CUSTOMER) { UIHelper::drawText(window, font, "Access Denied", 400, 300, 30, Color(180, 0, 0)); return; }
        float y = 105;
        UIHelper::drawText(window, font, "Order Status: Pending -> Preparing -> Ready -> Served", 15, y + 5, 14, Color(80, 40, 0));
        float rowY = y + 35;
        for (int i = kitchenScrollOffset; i < orderCount && i < kitchenScrollOffset + 8; i++) {
            int st = orders[i].getStatus();
            Color bg = (st == STATUS_PENDING) ? Color(255, 240, 180)
                : (st == STATUS_PREPARING) ? Color(180, 230, 255)
                : (st == STATUS_READY) ? Color(180, 255, 180)
                : Color(220, 220, 220);
            UIHelper::drawRect(window, 5, rowY, WINDOW_WIDTH - 10, 58, bg, Color(100, 100, 100), 1);
            UIHelper::drawText(window, font,
                "Order #" + intToStr(orders[i].getOrderId()) + "  Table:" + intToStr(orders[i].getTableNumber()) + "  Customer: " + orders[i].getCustomerName(),
                15, rowY + 3, 15, Color(20, 20, 20));
            string itemsStr = "Items: ";
            for (int k = 0; k < orders[i].getItemCount(); k++)
                itemsStr += orders[i].getItem(k).getItem().getItemName() + "x" + intToStr(orders[i].getItem(k).getQuantity()) + " ";
            UIHelper::drawText(window, font, itemsStr, 15, rowY + 22, 12, Color(40, 40, 40));
            UIHelper::drawRect(window, WINDOW_WIDTH - 290, rowY + 8, 130, 30, Color(50, 50, 50));
            UIHelper::drawText(window, font, "Status: " + statusToStr(st), WINDOW_WIDTH - 288, rowY + 12, 14, Color::Yellow);
            if (st != STATUS_SERVED)
                UIHelper::drawButton(window, font, "Advance->", WINDOW_WIDTH - 150, rowY + 8, 140, 30, Color(30, 100, 30), Color::White, 14);
            rowY += 65;
        }
        if (orderCount == 0) UIHelper::drawText(window, font, "No active orders in kitchen.", 300, y + 60, 18, Color(120, 120, 120));
        UIHelper::drawButton(window, font, "^ Up", 10, WINDOW_HEIGHT - 80, 100, 30, Color(80, 80, 80), Color::White);
        UIHelper::drawButton(window, font, "v Down", 120, WINDOW_HEIGHT - 80, 100, 30, Color(80, 80, 80), Color::White);
    }

    // Submit screen for feedback; right panel shows all past feedback entries
    void drawFeedbackScreen() {
        drawTopBar("Customer Feedback & Ratings"); drawNavBar();
        float y = 105; Customer* c = currentCustomer();
        UIHelper::drawRect(window, 5, y, 310, 280, Color(245, 255, 245), Color(0, 130, 0), 1);
        UIHelper::drawText(window, font, "Submit Feedback", 15, y + 5, 16, Color(0, 80, 0));
        string fbName = (currentRole == ROLE_CUSTOMER && c) ? c->getName() : feedbackCustInput;
        drawInputBox(15, y + 35, 290, 32, "Customer Name:", fbName, typingFbCust);
        drawInputBox(15, y + 100, 290, 32, "Rating (1-5):", feedbackRatingInput, typingFbRating);
        drawInputBox(15, y + 165, 290, 32, "Comment (min 3 chars):", feedbackCommentInput, typingFbComment);
        UIHelper::drawButton(window, font, "Submit", 80, y + 220, 160, 38, Color(0, 120, 0), Color::White);
        UIHelper::drawRect(window, 325, y, WINDOW_WIDTH - 335, 530, Color(248, 255, 248), Color(0, 150, 0), 1);
        UIHelper::drawText(window, font, "Recent Feedback (" + intToStr(feedbackCount) + " entries):", 335, y + 5, 15, Color(0, 80, 0));
        float rowY = y + 30;
        for (int i = 0; i < feedbackCount; i++) {
            Color rc = (feedbacks[i].getRating() >= 4) ? Color(0, 150, 0)
                : (feedbacks[i].getRating() >= 3) ? Color(150, 130, 0) : Color(180, 0, 0);
            UIHelper::drawRect(window, 328, rowY, WINDOW_WIDTH - 340, 50, Color(240, 255, 240), Color(0, 130, 0), 1);
            UIHelper::drawText(window, font, feedbacks[i].getCustomerName() + " - Rating: " + intToStr(feedbacks[i].getRating()) + "/5", 335, rowY + 3, 14, rc);
            UIHelper::drawText(window, font, "Comment: " + feedbacks[i].getComment() + " | " + feedbacks[i].getDate(), 335, rowY + 22, 12, Color(50, 50, 50));
            rowY += 57; if (rowY > y + 510) break;
        }
        if (feedbackCount == 0) UIHelper::drawText(window, font, "No feedback yet.", 400, y + 50, 16, Color(120, 120, 120));
    }

    // Add a party to the waiting queue; staff can seat (remove) the first group
    void drawQueueScreen() {
        drawTopBar("Waiting Queue"); drawNavBar();
        float y = 105;
        UIHelper::drawRect(window, 5, y, 310, 310, Color(255, 248, 240), Color(180, 80, 0), 1);
        UIHelper::drawText(window, font, "Add to Waiting Queue", 15, y + 5, 15, Color(120, 50, 0));
        drawInputBox(15, y + 35, 290, 32, "Party Name (letters only):", queueNameInput, typingQueueName);
        drawInputBox(15, y + 100, 290, 32, "Party Size (1-20):", queueSizeInput, typingQueueSize);
        drawInputBox(15, y + 165, 290, 32, "Arrival Time (e.g. 7PM):", queueTimeInput, typingQueueTime);
        UIHelper::drawButton(window, font, "Add to Queue", 60, y + 225, 190, 38, Color(160, 80, 0), Color::White);
        UIHelper::drawRect(window, 325, y, WINDOW_WIDTH - 335, 530, Color(255, 252, 245), Color(180, 100, 0), 1);
        UIHelper::drawText(window, font, "Current Queue (" + intToStr(queueCount) + " groups):", 335, y + 5, 15, Color(120, 50, 0));
        float rowY = y + 30;
        for (int i = 0; i < queueCount; i++) {
            Color bg = (i % 2 == 0) ? Color(255, 240, 220) : Color(255, 250, 240);
            UIHelper::drawRect(window, 328, rowY, WINDOW_WIDTH - 340, 50, bg, Color(200, 140, 0), 1);
            UIHelper::drawText(window, font, intToStr(i + 1) + ". " + waitQueue[i].getName() + " | Party: " + intToStr(waitQueue[i].getPartySize()), 335, rowY + 3, 14, Color(80, 40, 0));
            UIHelper::drawText(window, font, "Arrived: " + waitQueue[i].getArrivalTime(), 335, rowY + 24, 12, Color(100, 60, 0));
            UIHelper::drawButton(window, font, "Seat", WINDOW_WIDTH - 90, rowY + 10, 60, 28, Color(0, 120, 60), Color::White, 13);
            rowY += 57; if (rowY > y + 510) break;
        }
        if (queueCount == 0) UIHelper::drawText(window, font, "No groups waiting.", 400, y + 40, 16, Color(120, 120, 120));
    }

    // ── Event handlers ────────────────────────────────────────
    // Each handler matches the draw method for the same screen.

    void handleLoginEvents(Event& event) {
        float cx = WINDOW_WIDTH / 2 - 210, cy = 175;
        if (!showRegisterForm) {
            if (clicked(event, cx + 120, cy, 100, 30)) { loginAsCustomer = false; loginUsernameInput = loginPasswordInput = ""; }
            if (clicked(event, cx + 228, cy, 120, 30)) { loginAsCustomer = true;  loginUsernameInput = loginPasswordInput = ""; }
            if (clicked(event, cx + 10, cy + 80, 390, 38)) { typingUsername = true;  typingPassword = false; }
            if (clicked(event, cx + 10, cy + 155, 390, 38)) { typingUsername = false; typingPassword = true; }
            handleTextInput(event, loginUsernameInput, typingUsername);
            handleTextInput(event, loginPasswordInput, typingPassword);
            if (clicked(event, cx + 115, cy + 225, 180, 45)) attemptLogin();
            if (event.type == Event::KeyPressed && event.key.code == Keyboard::Return) attemptLogin();
            if (loginAsCustomer && clicked(event, cx + 90, cy + 305, 220, 32)) {
                showRegisterForm = true; typingUsername = typingPassword = false;
            }
        }
        else {
            if (clicked(event, cx + 10, cy + 38, 390, 34)) { typingRegName = true;  typingRegPhone = typingRegUser = typingRegPass = false; }
            if (clicked(event, cx + 10, cy + 108, 390, 34)) { typingRegPhone = true;  typingRegName = typingRegUser = typingRegPass = false; }
            if (clicked(event, cx + 10, cy + 178, 390, 34)) { typingRegUser = true;  typingRegName = typingRegPhone = typingRegPass = false; }
            if (clicked(event, cx + 10, cy + 248, 390, 34)) { typingRegPass = true;  typingRegName = typingRegPhone = typingRegUser = false; }
            handleTextInput(event, regNameInput, typingRegName);
            handlePhoneInput(event, regPhoneInput, typingRegPhone);
            handleTextInput(event, regUserInput, typingRegUser);
            handleTextInput(event, regPassInput, typingRegPass);

            if (clicked(event, cx + 100, cy + 308, 200, 40)) {
                if (regNameInput.empty() || regPhoneInput.empty() || regUserInput.empty() || regPassInput.empty())
                {
                    showStatus("All fields are required."); return;
                }
                if (!isValidName(regNameInput))
                {
                    showStatus("Name must contain letters and spaces only (min 2 chars)."); return;
                }
                if ((int)regPhoneInput.size() != 11)
                {
                    showStatus("Phone must be exactly 11 digits."); return;
                }
                if (isPhoneAllSame(regPhoneInput))
                {
                    showStatus("Phone cannot have all identical digits."); return;
                }
                if (!isValidUsername(regUserInput))
                {
                    showStatus("Username: letters, digits, underscore only."); return;
                }
                if ((int)regPassInput.size() < 8)
                {
                    showStatus("Password must be at least 8 characters."); return;
                }
                for (int i = 0; i < customerCount; i++)
                    if (customers[i].getUsername() == regUserInput)
                    {
                        showStatus("Username already taken."); return;
                    }
                if (customerCount >= MAX_CUSTOMERS)
                {
                    showStatus("Customer limit reached."); return;
                }
                customers[customerCount] = Customer(nextCustomerId++, regNameInput, regPhoneInput, regUserInput, regPassInput);
                customerPasswords[customerCount] = regPassInput;
                customerCount++;
                FileHandler::saveCustomersFull(customers, customerCount, customerPasswords);
                showStatus("Account created! Login as: " + regUserInput);
                regNameInput = regPhoneInput = regUserInput = regPassInput = "";
                showRegisterForm = false; loginAsCustomer = true;
            }
            if (clicked(event, cx + 100, cy + 360, 200, 32)) {
                showRegisterForm = false;
                regNameInput = regPhoneInput = regUserInput = regPassInput = "";
            }
        }
    }

    // Checks credentials against staff or customer arrays and sets session state
    void attemptLogin() {
        if (!loginAsCustomer) {
            for (int i = 0; i < staffCount; i++) {
                if (staff[i].getUsername() == loginUsernameInput && staff[i].checkPassword(loginPasswordInput)) {
                    loggedIn = true; currentRole = ROLE_STAFF;
                    currentStaffIdx = i; currentCustomerIdx = -1;
                    currentScreen = SCR_HOME;
                    showStatus("Welcome, " + staff[i].getName() + "! (Admin)");
                    loginUsernameInput = loginPasswordInput = ""; return;
                }
            }
            showStatus("Invalid admin credentials."); loginPasswordInput = "";
        }
        else {
            for (int i = 0; i < customerCount; i++) {
                if (customers[i].hasLogin() &&
                    customers[i].getUsername() == loginUsernameInput &&
                    customers[i].checkPassword(loginPasswordInput))
                {
                    loggedIn = true; currentRole = ROLE_CUSTOMER;
                    currentCustomerIdx = i; currentStaffIdx = -1;
                    currentScreen = SCR_HOME;
                    showStatus("Welcome back, " + customers[i].getName() + "!");
                    loginUsernameInput = loginPasswordInput = ""; return;
                }
            }
            showStatus("Invalid customer credentials."); loginPasswordInput = "";
        }
    }

    void handleHomeEvents(Event& event) {
        handleNavBarClick(event);
        float startY = 110, cardY = startY + 80, cardH = 100, btnY = cardY + cardH + 133;
        if (currentRole == ROLE_STAFF) {
            if (clicked(event, 10, btnY, 160, 40)) currentScreen = SCR_BILLING;
            if (clicked(event, 180, btnY, 160, 40)) currentScreen = SCR_RESERVATION;
            if (clicked(event, 350, btnY, 160, 40)) currentScreen = SCR_QUEUE;
        }
        else {
            if (clicked(event, 10, btnY, 160, 40)) currentScreen = SCR_MENU;
            if (clicked(event, 180, btnY, 160, 40)) currentScreen = SCR_ORDER;
            if (clicked(event, 350, btnY, 160, 40)) currentScreen = SCR_BILLING;
            if (clicked(event, 520, btnY, 160, 40)) currentScreen = SCR_RESERVATION;
        }
        // Logout: clear session state and return to login screen
        if (clicked(event, WINDOW_WIDTH - 130, WINDOW_HEIGHT - 50, 120, 35)) {
            loggedIn = false; currentRole = ROLE_NONE;
            currentStaffIdx = -1; currentCustomerIdx = -1;
            currentScreen = SCR_LOGIN; loginAsCustomer = false;
            showStatus("Logged out successfully.");
        }
    }

    void handleMenuEvents(Event& event) {
        handleNavBarClick(event);
        float panelY = 105;
        if (currentRole == ROLE_STAFF) {
            if (clicked(event, 15, panelY + 35, 290, 32)) { typingMenuName = true;  typingMenuPrice = typingMenuCat = typingMenuEditPrice = false; }
            if (clicked(event, 15, panelY + 100, 290, 32)) { typingMenuPrice = true;  typingMenuName = typingMenuCat = typingMenuEditPrice = false; }
            if (clicked(event, 15, panelY + 165, 290, 32)) { typingMenuCat = true;  typingMenuName = typingMenuPrice = typingMenuEditPrice = false; }
            if (clicked(event, 15, panelY + 290, 200, 30)) { typingMenuEditPrice = true; typingMenuName = typingMenuPrice = typingMenuCat = false; }
            handleTextInput(event, menuNameInput, typingMenuName);
            handleNumericInput(event, menuPriceInput, typingMenuPrice, 8);
            handleTextInput(event, menuCatInput, typingMenuCat);
            handleNumericInput(event, menuEditPriceInput, typingMenuEditPrice, 8);

            if (clicked(event, 80, panelY + 210, 160, 35)) {
                if (menuNameInput.empty() || menuPriceInput.empty() || menuCatInput.empty())
                {
                    showStatus("Fill all fields to add item."); return;
                }
                if (!isValidName(menuNameInput))
                {
                    showStatus("Item name: letters and spaces only (min 2 chars)."); return;
                }
                if (!isValidCategory(menuCatInput))
                {
                    showStatus("Category: letters and spaces only (min 2 chars)."); return;
                }
                float parsedPrice;
                if (!isValidPrice(menuPriceInput, parsedPrice))
                {
                    showStatus("Price must be a number between 1 and 99999."); return;
                }
                if (menuNameExists(menuNameInput))
                {
                    showStatus("Item '" + menuNameInput + "' already exists."); return;
                }
                if (menuCount >= MAX_MENU)
                {
                    showStatus("Menu is full."); return;
                }
                menu[menuCount++] = MenuItem(nextMenuId++, menuNameInput, parsedPrice, menuCatInput);
                FileHandler::saveMenu(menu, menuCount);
                showStatus("Added: " + menuNameInput + " @ Rs." + floatToStr(parsedPrice));
                menuNameInput = menuPriceInput = menuCatInput = "";
            }
            if (selectedMenuItemForEdit >= 0 && selectedMenuItemForEdit < menuCount) {
                if (clicked(event, 220, panelY + 290, 90, 30)) {
                    if (menuEditPriceInput.empty()) { showStatus("Enter new price first."); return; }
                    float np;
                    if (!isValidPrice(menuEditPriceInput, np)) { showStatus("Invalid price."); return; }
                    menu[selectedMenuItemForEdit].setPrice(np);
                    FileHandler::saveMenu(menu, menuCount);
                    showStatus("Price updated: " + menu[selectedMenuItemForEdit].getItemName());
                    menuEditPriceInput = ""; selectedMenuItemForEdit = -1;
                }
            }
        }

        int visible = 16; float itemY = panelY + 28;
        for (int i = menuScrollOffset; i < menuCount && i < menuScrollOffset + visible; i++) {
            float rowY = itemY + (i - menuScrollOffset) * 30;
            if (clicked(event, 326, rowY, WINDOW_WIDTH - 337 - 45, 28))
                selectedMenuItemForEdit = (selectedMenuItemForEdit == i) ? -1 : i;
            if (currentRole == ROLE_STAFF) {
                if (clicked(event, WINDOW_WIDTH - 60, rowY + 2, 40, 24)) {
                    showStatus("Removed: " + menu[i].getItemName());
                    if (selectedMenuItemForEdit == i)     selectedMenuItemForEdit = -1;
                    else if (selectedMenuItemForEdit > i) selectedMenuItemForEdit--;
                    for (int j = i; j < menuCount - 1; j++) menu[j] = menu[j + 1];
                    menuCount--;
                    FileHandler::saveMenu(menu, menuCount);
                    clampScrollOffsets(); break;
                }
            }
        }
        if (clicked(event, WINDOW_WIDTH - 80, panelY + 465, 75, 28) && menuScrollOffset > 0)              menuScrollOffset--;
        if (clicked(event, WINDOW_WIDTH - 80, panelY + 498, 75, 28) && menuScrollOffset < menuCount - 16) menuScrollOffset++;
        if (event.type == Event::MouseWheelScrolled) {
            if (event.mouseWheelScroll.delta > 0 && menuScrollOffset > 0)              menuScrollOffset--;
            if (event.mouseWheelScroll.delta < 0 && menuScrollOffset < menuCount - 16) menuScrollOffset++;
        }
        clampScrollOffsets();
    }

    void handleOrderEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105; Customer* c = currentCustomer();
        if (clicked(event, 15, y + 35, 300, 32)) typingOrderCustomer = true;
        else if (event.type == Event::MouseButtonPressed) typingOrderCustomer = false;
        if (currentRole == ROLE_CUSTOMER && c) orderCustomerInput = c->getName();
        else handleTextInput(event, orderCustomerInput, typingOrderCustomer);

        for (int i = 0; i < TOTAL_TABLES; i++) {
            float tx = 15 + (i % 5) * 58, ty = y + 100 + (i / 5) * 38;
            if (clicked(event, tx, ty, 50, 30)) {
                string custName = (currentRole == ROLE_CUSTOMER && c) ? c->getName() : orderCustomerInput;
                if (tables[i].isReserved() && tables[i].getReservedFor() != custName)
                    showStatus("Table " + intToStr(i + 1) + " is reserved for " + tables[i].getReservedFor() + " only!");
                else
                    selectedTableForOrder = i + 1;
            }
        }
        for (int i = orderMenuScroll; i < menuCount && i < orderMenuScroll + 8; i++) {
            float ry = y + 200 + (i - orderMenuScroll) * 26;
            if (clicked(event, 15, ry, 295, 24)) selectedMenuItemIndex = i;
        }
        if (clicked(event, 90, y + 408, 60, 28)) typingOrderQty = true;
        else if (event.type == Event::MouseButtonPressed) typingOrderQty = false;
        handleNumericInput(event, orderQtyStr, typingOrderQty, 3);

        string custName = (currentRole == ROLE_CUSTOMER && c) ? c->getName() : orderCustomerInput;

        if (clicked(event, 160, y + 408, 140, 28)) {
            if (custName.empty()) { showStatus("Enter customer name first!"); return; }
            if (!isValidName(custName)) { showStatus("Customer name: letters and spaces only."); return; }
            if (menuCount == 0) { showStatus("Menu is empty."); return; }
            if (tableReservedBySomeoneElse(selectedTableForOrder - 1, custName)) { showStatus("Table reserved by someone else!"); return; }
            int qty = 1;
            if (!orderQtyStr.empty() && !isValidQty(orderQtyStr, qty)) { showStatus("Quantity must be 1-50."); return; }
            if (activeOrderIdx < 0) {
                if (orderCount >= MAX_ORDERS) { showStatus("Order limit reached."); return; }
                orders[orderCount] = Order(nextOrderId++, selectedTableForOrder, custName);
                activeOrderIdx = orderCount; orderCount++;
                FileHandler::saveNextOrderId(nextOrderId);
            }
            if (selectedMenuItemIndex >= 0 && selectedMenuItemIndex < menuCount)
                orders[activeOrderIdx].addItem(menu[selectedMenuItemIndex], qty);
            else
                showStatus("Select a menu item first.");
        }

        if (activeOrderIdx >= 0 && activeOrderIdx < orderCount) {
            float rowY = y + 55;
            for (int i = 0; i < orders[activeOrderIdx].getItemCount(); i++) {
                if (clicked(event, WINDOW_WIDTH - 55, rowY - 3, 30, 22)) {
                    orders[activeOrderIdx].removeItem(orders[activeOrderIdx].getItem(i).getItem().getItemId());
                    showStatus("Item removed."); break;
                }
                rowY += 28;
            }
        }

        if (clicked(event, 20, y + 450, 280, 40)) {
            // If all tables are reserved, add the customer to the waiting queue instead
            bool allReserved = true;
            for (int i = 0; i < TOTAL_TABLES; i++) if (!tables[i].isReserved()) { allReserved = false; break; }
            if (allReserved) {
                string custName2 = (currentRole == ROLE_CUSTOMER && c) ? c->getName() : orderCustomerInput;
                if (custName2.empty()) { showStatus("Enter customer name first!"); return; }
                if (queueCount >= MAX_QUEUE) { showStatus("Queue is full!"); return; }
                if (queueNameExists(custName2)) { showStatus(custName2 + " is already in the queue!"); return; }
                waitQueue[queueCount++] = WaitingEntry(custName2, 1, getTodayDate());
                FileHandler::saveQueue(waitQueue, queueCount);
                showStatus("All tables reserved! " + custName2 + " added to queue.");
                activeOrderIdx = -1; orderCustomerInput = ""; return;
            }
            if (activeOrderIdx < 0 || activeOrderIdx >= orderCount) { showStatus("No active order."); return; }
            if (orders[activeOrderIdx].getItemCount() == 0) { showStatus("Add at least one item first."); return; }
            showStatus("Order #" + intToStr(orders[activeOrderIdx].getOrderId()) + " confirmed!");
            FileHandler::saveNextOrderId(nextOrderId);
            activeOrderIdx = -1; orderCustomerInput = "";
        }
        if (clicked(event, 290, y + 200, 25, 25) && orderMenuScroll > 0)             orderMenuScroll--;
        if (clicked(event, 290, y + 410, 25, 25) && orderMenuScroll < menuCount - 8) orderMenuScroll++;
    }

    void handleBillingEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105; Customer* c = currentCustomer();
        if (currentRole == ROLE_STAFF) {
            int unpaid = 0;
            for (int i = 0; i < orderCount; i++) {
                if (!orders[i].isPaid()) {
                    if (clicked(event, 520, y + 40 + unpaid * 45 + 6, 70, 26)) selectedOrderForBill = i;
                    unpaid++;
                }
            }
            if (selectedOrderForBill >= 0 && selectedOrderForBill < orderCount && !orders[selectedOrderForBill].isPaid()) {
                if (clicked(event, WINDOW_WIDTH / 2 - 150, y + 330, 120, 60)) {
                    Order& ord = orders[selectedOrderForBill];
                    ord.markPaid(PAY_CASH);
                    FileHandler::saveOrderHistory(ord);
                    updateCustomerAfterPayment(ord);
                    showStatus("Bill confirmed paid!"); selectedOrderForBill = 0;
                }
                if (clicked(event, WINDOW_WIDTH / 2 + 30, y + 330, 120, 60))
                    showStatus("Bill not confirmed.");
            }
            return;
        }
        int unpaid = 0;
        for (int i = 0; i < orderCount; i++) {
            if (!orders[i].isPaid() && c && orders[i].getCustomerName() == c->getName()) {
                if (clicked(event, 520, y + 30 + unpaid * 50 + 7, 70, 26)) selectedOrderForBill = i;
                unpaid++;
            }
        }
        if (selectedOrderForBill >= 0 && selectedOrderForBill < orderCount) {
            Order& ord = orders[selectedOrderForBill];
            float by = y + 300;
            if (!ord.isAwaitingCashConfirm()) {
                if (clicked(event, 20, by, 160, 40)) { ord.requestCashPayment(); showStatus("Awaiting staff..."); }
                if (clicked(event, 200, by, 160, 40)) { ord.markPaid(PAY_CARD); FileHandler::saveOrderHistory(ord); updateCustomerAfterPayment(ord); showStatus("Card paid!"); }
            }
        }
    }

    void handleReservationEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105; Customer* c = currentCustomer();
        if (currentRole == ROLE_CUSTOMER && c) {
            if (clicked(event, 15, y + 20, 290, 32)) typingResTime = true;
            handleTextInput(event, reserveTimeInput, typingResTime);
            for (int i = 0; i < TOTAL_TABLES; i++) {
                float tx = 15 + (i % 5) * 58, ty = y + 85 + (i / 5) * 38;
                if (clicked(event, tx, ty, 50, 30)) selectedTableForRes = i + 1;
            }
            if (clicked(event, 60, y + 165, 160, 35)) {
                if (reserveTimeInput.empty()) { showStatus("Enter time first."); return; }
                if (!isValidTimeSlot(reserveTimeInput)) { showStatus("Invalid time! e.g. 7PM"); return; }
                if (selectedTableForRes < 1) { showStatus("Select a table."); return; }
                Table& tbl = tables[selectedTableForRes - 1];
                if (tbl.isReserved() && tbl.getReservedFor() != c->getName()) { showStatus("Table already reserved!"); return; }
                tbl.reserve(c->getName(), reserveTimeInput);
                FileHandler::saveReservations(tables, TOTAL_TABLES);
                showStatus("Table " + intToStr(selectedTableForRes) + " reserved at " + reserveTimeInput);
                reserveTimeInput = "";
            }
        }
        if (currentRole == ROLE_STAFF) {
            float rowY = y + 220;
            for (int i = 0; i < TOTAL_TABLES; i++) {
                if (tables[i].isReserved()) {
                    if (clicked(event, WINDOW_WIDTH - 110, rowY - 3, 80, 25)) {
                        showStatus("Cancelled Table " + intToStr(tables[i].getTableNum()));
                        tables[i].cancelReservation();
                        FileHandler::saveReservations(tables, TOTAL_TABLES);
                    }
                    rowY += 35;
                }
            }
        }
    }

    void handleCustomerEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105;
        if (currentRole == ROLE_STAFF) {
            if (clicked(event, 15, y + 35, 290, 32)) { typingCustName = true;  typingCustPhone = false; }
            if (clicked(event, 15, y + 100, 290, 32)) { typingCustName = false; typingCustPhone = true; }
            handleTextInput(event, custNameInput, typingCustName);
            handlePhoneInput(event, custPhoneInput, typingCustPhone);
            if (clicked(event, 60, y + 155, 190, 35)) {
                if (custNameInput.empty() || custPhoneInput.empty()) { showStatus("Enter both name and phone."); return; }
                if (!isValidName(custNameInput)) { showStatus("Name: letters and spaces only (min 2)."); return; }
                if ((int)custPhoneInput.size() != 11) { showStatus("Phone must be exactly 11 digits."); return; }
                if (isPhoneAllSame(custPhoneInput)) { showStatus("Phone cannot have all identical digits."); return; }
                if (customerCount >= MAX_CUSTOMERS) { showStatus("Customer limit reached."); return; }
                customers[customerCount] = Customer(nextCustomerId++, custNameInput, custPhoneInput);
                customerPasswords[customerCount] = "";
                customerCount++;
                FileHandler::saveCustomersFull(customers, customerCount, customerPasswords);
                showStatus("Customer added: " + custNameInput);
                custNameInput = custPhoneInput = "";
            }
        }
        if (clicked(event, WINDOW_WIDTH - 80, y + 490, 75, 30) && custScrollOffset > 0)                custScrollOffset--;
        if (clicked(event, WINDOW_WIDTH - 80, y + 525, 75, 30) && custScrollOffset < customerCount - 15) custScrollOffset++;
        clampScrollOffsets();
    }

    void handleReportEvents(Event& event) { handleNavBarClick(event); }

    void handleKitchenEvents(Event& event) {
        handleNavBarClick(event);
        if (currentRole != ROLE_STAFF) return;
        float y = 105, rowY = y + 35;
        for (int i = kitchenScrollOffset; i < orderCount && i < kitchenScrollOffset + 8; i++) {
            if (orders[i].getStatus() != STATUS_SERVED)
                if (clicked(event, WINDOW_WIDTH - 150, rowY + 8, 140, 30)) {
                    orders[i].advanceStatus();
                    showStatus("Order #" + intToStr(orders[i].getOrderId()) + " -> " + statusToStr(orders[i].getStatus()));
                }
            rowY += 65;
        }
        if (clicked(event, 10, WINDOW_HEIGHT - 80, 100, 30) && kitchenScrollOffset > 0)              kitchenScrollOffset--;
        if (clicked(event, 120, WINDOW_HEIGHT - 80, 100, 30) && kitchenScrollOffset < orderCount - 8) kitchenScrollOffset++;
        clampScrollOffsets();
    }

    void handleFeedbackEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105; Customer* c = currentCustomer();
        if (currentRole == ROLE_CUSTOMER && c) feedbackCustInput = c->getName();
        else {
            if (clicked(event, 15, y + 35, 290, 32)) { typingFbCust = true; typingFbRating = typingFbComment = false; }
            handleTextInput(event, feedbackCustInput, typingFbCust);
        }
        if (clicked(event, 15, y + 100, 290, 32)) { typingFbRating = true;  typingFbCust = typingFbComment = false; }
        if (clicked(event, 15, y + 165, 290, 32)) { typingFbComment = true;  typingFbCust = typingFbRating = false; }
        // Rating only accepts a single digit 1-5; backspace clears it entirely
        if (typingFbRating && event.type == Event::TextEntered) {
            if (event.text.unicode == 8) feedbackRatingInput = "";
            else if (feedbackRatingInput.empty()) {
                char ch = (char)event.text.unicode;
                if (ch >= '1' && ch <= '5') feedbackRatingInput += ch;
                else showStatus("Rating must be 1-5.");
            }
        }
        handleTextInput(event, feedbackCommentInput, typingFbComment);
        if (clicked(event, 80, y + 220, 160, 38)) {
            string name = (currentRole == ROLE_CUSTOMER && c) ? c->getName() : feedbackCustInput;
            if (name.empty() || feedbackRatingInput.empty() || feedbackCommentInput.empty()) { showStatus("All fields required."); return; }
            if (!isValidName(name)) { showStatus("Name: letters and spaces only."); return; }
            if (!isValidComment(feedbackCommentInput)) { showStatus("Comment min 3 chars."); return; }
            int rating; safeStoi(feedbackRatingInput, rating);
            if (rating < 1 || rating > 5) { showStatus("Rating must be 1-5."); return; }
            if (feedbackCount >= MAX_FEEDBACKS) { showStatus("Feedback limit reached."); return; }
            feedbacks[feedbackCount++] = FeedbackRecord(name, rating, feedbackCommentInput);
            for (int i = 0; i < customerCount; i++)
                if (customers[i].getName() == name) {
                    customers[i].setFeedback(rating);
                    FileHandler::saveCustomersFull(customers, customerCount, customerPasswords); break;
                }
            FileHandler::saveFeedbacks(feedbacks, feedbackCount);
            showStatus("Feedback submitted! Rating: " + intToStr(rating) + "/5");
            feedbackCustInput = feedbackRatingInput = feedbackCommentInput = "";
        }
    }

    void handleQueueEvents(Event& event) {
        handleNavBarClick(event);
        float y = 105;
        if (clicked(event, 15, y + 35, 290, 32)) { typingQueueName = true;  typingQueueSize = typingQueueTime = false; }
        if (clicked(event, 15, y + 100, 290, 32)) { typingQueueSize = true;  typingQueueName = typingQueueTime = false; }
        if (clicked(event, 15, y + 165, 290, 32)) { typingQueueTime = true;  typingQueueName = typingQueueSize = false; }
        handleTextInput(event, queueNameInput, typingQueueName);
        handleNumericInput(event, queueSizeInput, typingQueueSize, 2);
        handleTextInput(event, queueTimeInput, typingQueueTime);
        if (clicked(event, 60, y + 225, 190, 38)) {
            if (queueNameInput.empty() || queueSizeInput.empty() || queueTimeInput.empty()) { showStatus("Fill all queue fields."); return; }
            if (!isValidName(queueNameInput)) { showStatus("Party name: letters and spaces only."); return; }
            int ps;
            if (!isValidPartySize(queueSizeInput, ps)) { showStatus("Party size must be 1-20."); return; }
            if (!isValidTimeSlot(queueTimeInput)) { showStatus("Invalid time! e.g. 7PM"); return; }
            if (queueNameExists(queueNameInput)) { showStatus("'" + queueNameInput + "' already in queue."); return; }
            if (queueCount >= MAX_QUEUE) { showStatus("Queue is full."); return; }
            waitQueue[queueCount++] = WaitingEntry(queueNameInput, ps, queueTimeInput);
            FileHandler::saveQueue(waitQueue, queueCount);
            showStatus(queueNameInput + " (party of " + intToStr(ps) + ") added.");
            queueNameInput = queueSizeInput = queueTimeInput = "";
        }
        float rowY = y + 30;
        for (int i = 0; i < queueCount; i++) {
            if (clicked(event, WINDOW_WIDTH - 90, rowY + 10, 60, 28)) {
                showStatus(waitQueue[i].getName() + " seated!");
                for (int j = i; j < queueCount - 1; j++) waitQueue[j] = waitQueue[j + 1];
                queueCount--;
                FileHandler::saveQueue(waitQueue, queueCount); break;
            }
            rowY += 57; if (rowY > y + 510) break;
        }
    }

    // Routes events and draw calls to the correct screen handler
    void handleEvents(Event& event) {
        switch (currentScreen) {
        case SCR_LOGIN:       handleLoginEvents(event);       break;
        case SCR_HOME:        handleHomeEvents(event);        break;
        case SCR_MENU:        handleMenuEvents(event);        break;
        case SCR_ORDER:       handleOrderEvents(event);       break;
        case SCR_BILLING:     handleBillingEvents(event);     break;
        case SCR_RESERVATION: handleReservationEvents(event); break;
        case SCR_CUSTOMERS:   handleCustomerEvents(event);    break;
        case SCR_REPORTS:     handleReportEvents(event);      break;
        case SCR_KITCHEN:     handleKitchenEvents(event);     break;
        case SCR_FEEDBACK:    handleFeedbackEvents(event);    break;
        case SCR_QUEUE:       handleQueueEvents(event);       break;
        }
    }
    void drawCurrentScreen() {
        switch (currentScreen) {
        case SCR_LOGIN:       drawLoginScreen();       break;
        case SCR_HOME:        drawHomeScreen();        break;
        case SCR_MENU:        drawMenuScreen();        break;
        case SCR_ORDER:       drawOrderScreen();       break;
        case SCR_BILLING:     drawBillingScreen();     break;
        case SCR_RESERVATION: drawReservationScreen(); break;
        case SCR_CUSTOMERS:   drawCustomerScreen();    break;
        case SCR_REPORTS:     drawReportsScreen();     break;
        case SCR_KITCHEN:     drawKitchenScreen();     break;
        case SCR_FEEDBACK:    drawFeedbackScreen();    break;
        case SCR_QUEUE:       drawQueueScreen();       break;
        }
    }

public:
    // Constructor: initializes all state, loads saved data, and seeds the menu if empty
    RestaurantApp()
        : window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bundu Khan Restaurant - Management System"),
        currentScreen(SCR_LOGIN), currentRole(ROLE_NONE), loggedIn(false),
        currentStaffIdx(-1), currentCustomerIdx(-1),
        menuCount(0), customerCount(0), orderCount(0), staffCount(0),
        feedbackCount(0), queueCount(0),
        nextMenuId(1), nextOrderId(1), nextCustomerId(1),
        typingUsername(false), typingPassword(false), loginAsCustomer(false),
        typingRegName(false), typingRegPhone(false), typingRegUser(false), typingRegPass(false),
        showRegisterForm(false), menuScrollOffset(0),
        typingMenuName(false), typingMenuPrice(false), typingMenuCat(false),
        selectedMenuItemForEdit(-1), typingMenuEditPrice(false),
        selectedTableForOrder(1), orderMenuScroll(0),
        typingOrderCustomer(false), activeOrderIdx(-1),
        orderQtyStr("1"), typingOrderQty(false), selectedMenuItemIndex(0),
        selectedOrderForBill(0),
        reserveNameInput(""), reserveTimeInput(""), selectedTableForRes(1),
        typingResName(false), typingResTime(false),
        typingCustName(false), typingCustPhone(false), custScrollOffset(0),
        kitchenScrollOffset(0),
        typingFbCust(false), typingFbRating(false), typingFbComment(false),
        typingQueueName(false), typingQueueSize(false), typingQueueTime(false),
        verifiedRevenue(0), rawRevenue(0), corruptRecords(0), missingRecords(0),
        integrityChecked(false)
    {
        window.setFramerateLimit(60);
        gameView.setSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        gameView.setCenter(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f);
        window.setView(gameView);

        // Try common system font paths; fall back to arial.ttf in the working directory
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
            if (!font.loadFromFile("/usr/share/fonts/TTF/DejaVuSans.ttf"))
                font.loadFromFile("arial.ttf");

        FileHandler::ensureDataFolder();
        menuCount = FileHandler::loadMenu(menu, MAX_MENU);
        customerCount = FileHandler::loadCustomers(customers, MAX_CUSTOMERS, customerPasswords);
        feedbackCount = FileHandler::loadFeedbacks(feedbacks, MAX_FEEDBACKS);

        // Initialize tables with varying capacities
        int caps[] = { 2,2,4,4,4,6,6,8,8,10 };
        for (int i = 0; i < TOTAL_TABLES; i++) tables[i] = Table(i + 1, caps[i]);
        FileHandler::loadReservations(tables, TOTAL_TABLES);

        // Hard-coded staff account (no file storage for staff in this version)
        staff[0] = Staff(2, "Waiter Bilal", "Waiter", "waiter1", "pass123", 35000);
        staffCount = 1;

        // Set next IDs to continue from where the last saved record left off
        if (menuCount > 0)     nextMenuId = menu[menuCount - 1].getItemId() + 1;
        if (customerCount > 0) nextCustomerId = customers[customerCount - 1].getId() + 1;
        nextOrderId = FileHandler::loadNextOrderId();
        queueCount = FileHandler::loadQueue(waitQueue, MAX_QUEUE);

        if (menuCount == 0) addSampleMenu();
    }

    // Main loop: polls events, clears the window, draws the current screen, then displays
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
