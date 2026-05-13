// UIHelper: Utility class for drawing UI elements using SFML
#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <string>
using namespace sf;
using namespace std;

class UIHelper {
public:

    // Draws a filled rectangle with an optional border
    static void drawRect(RenderWindow& win,
        float x, float y, float w, float h,
        Color color,
        Color outline = Color::Transparent,
        float outlineThick = 0)
    {
        RectangleShape rect(Vector2f(w, h));
        rect.setPosition(x, y);
        rect.setFillColor(color);
        rect.setOutlineColor(outline);
        rect.setOutlineThickness(outlineThick);
        win.draw(rect);
    }

    // Draws a text string at the given position with specified size and color
    static void drawText(RenderWindow& win, Font& font,
        const string& text, float x, float y,
        unsigned int size, Color color)
    {
        Text t;
        t.setFont(font);
        t.setString(text);
        t.setCharacterSize(size);
        t.setFillColor(color);
        t.setPosition(x, y);
        win.draw(t);
    }

    // Draws a button and returns true if the mouse is hovering over it.
    // The button lightens slightly on hover to give visual feedback.
    static bool drawButton(RenderWindow& win, Font& font,
        const string& label,
        float x, float y, float w, float h,
        Color bgColor, Color textColor,
        unsigned int textSize = 18)
    {
        // Convert mouse position to world coordinates
        Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

        // Check if the mouse is inside the button bounds
        bool hovering = (mousePos.x >= x && mousePos.x <= x + w &&
            mousePos.y >= y && mousePos.y <= y + h);

        // Add 30 to each color channel on hover, capped at 255
        Color dc = hovering
            ? Color(min(255, (int)bgColor.r + 30),
                min(255, (int)bgColor.g + 30),
                min(255, (int)bgColor.b + 30))
            : bgColor;

        drawRect(win, x, y, w, h, dc, Color(200, 200, 200), 1);

        // Measure the label size to center it inside the button
        Text temp;
        temp.setFont(font);
        temp.setString(label);
        temp.setCharacterSize(textSize);
        FloatRect tb = temp.getLocalBounds();

        drawText(win, font, label,
            x + (w - tb.width) / 2,
            y + (h - tb.height) / 2 - 5, // -5 corrects SFML's text baseline offset
            textSize, textColor);

        return hovering;
    }

    // Returns true if a left mouse click occurred inside the given rectangle.
    // Call this inside your event loop, not the draw loop.
    static bool isClicked(Event& event, RenderWindow& win,
        float x, float y, float w, float h)
    {
        if (event.type == Event::MouseButtonPressed &&
            event.mouseButton.button == Mouse::Left)
        {
            // Convert the click position to world coordinates
            Vector2f wp = win.mapPixelToCoords(
                Vector2i(event.mouseButton.x, event.mouseButton.y));

            return (wp.x >= x && wp.x <= x + w &&
                wp.y >= y && wp.y <= y + h);
        }
        return false;
    }
};
