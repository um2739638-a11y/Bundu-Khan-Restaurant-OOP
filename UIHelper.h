#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <string>
#include <algorithm>
#include "DataModels.h"

using namespace sf;

const int WINDOW_WIDTH = 1100;
const int WINDOW_HEIGHT = 700;

class UIHelper {
public:
    // ── Draw a filled rectangle with optional outline ──────────────────────
    static void drawRect(RenderWindow& win,
        float x, float y, float w, float h,
        Color color,
        Color outline = Color::Transparent,
        float outlineThick = 0) {
        RectangleShape rect(Vector2f(w, h));
        rect.setPosition(x, y);
        rect.setFillColor(color);
        rect.setOutlineColor(outline);
        rect.setOutlineThickness(outlineThick);
        win.draw(rect);
    }

    // ── Draw plain text ───────────────────────────────────────────────────
    static void drawText(RenderWindow& win, Font& font,
        const string& text,
        float x, float y,
        unsigned int size, Color color) {
        Text t;
        t.setFont(font);
        t.setString(text);
        t.setCharacterSize(size);
        t.setFillColor(color);
        t.setPosition(x, y);
        win.draw(t);
    }

    // ── Draw a button; returns true when mouse is hovering ────────────────
    static bool drawButton(RenderWindow& win, Font& font,
        const string& label,
        float x, float y, float w, float h,
        Color bgColor, Color textColor,
        unsigned int textSize = 18) {
        Vector2i rawMouse = Mouse::getPosition(win);
        Vector2f mousePos = win.mapPixelToCoords(rawMouse);

        bool hovering = (mousePos.x >= x && mousePos.x <= x + w &&
            mousePos.y >= y && mousePos.y <= y + h);

        Color displayColor = hovering
            ? Color(min(255, (int)bgColor.r + 30),
                min(255, (int)bgColor.g + 30),
                min(255, (int)bgColor.b + 30))
            : bgColor;

        drawRect(win, x, y, w, h, displayColor, Color(200, 200, 200), 1);

        Text temp;
        temp.setFont(font);
        temp.setString(label);
        temp.setCharacterSize(textSize);
        FloatRect textBounds = temp.getLocalBounds();
        float textX = x + (w - textBounds.width) / 2;
        float textY = y + (h - textBounds.height) / 2 - 5;
        drawText(win, font, label, textX, textY, textSize, textColor);
        return hovering;
    }

    // ── Hit-test for a mouse-press event ─────────────────────────────────
    static bool isClicked(Event& event, RenderWindow& win,
        float x, float y, float w, float h) {
        if (event.type == Event::MouseButtonPressed &&
            event.mouseButton.button == Mouse::Left) {
            Vector2f worldPos = win.mapPixelToCoords(
                Vector2i(event.mouseButton.x, event.mouseButton.y));
            return (worldPos.x >= x && worldPos.x <= x + w &&
                worldPos.y >= y && worldPos.y <= y + h);
        }
        return false;
    }
};