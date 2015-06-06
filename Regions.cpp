#include <SFML/Graphics.hpp>

#if defined (SFML_SYSTEM_WINDOWS)
    #include <windows.h>

    bool setTransparent(HWND hWnd, const sf::Image& image, unsigned char alpha)
    {
        const sf::Uint8* pixelData = image.getPixelsPtr();
        HRGN hRegion = CreateRectRgn(0, 0, image.getSize().x, image.getSize().y);

        // Determine the visible region
        for (unsigned int y = 0; y < image.getSize().y; y++)
        {
            for (unsigned int x = 0; x < image.getSize().x; x++)
            {
                if (pixelData[y * image.getSize().x * 4 + x * 4 + 3] == 0)
                {
                    HRGN hRegionDest = CreateRectRgn(0, 0, 1, 1);
                    HRGN hRegionPixel = CreateRectRgn(x, y, x+1, y+1);
                    CombineRgn(hRegionDest, hRegion, hRegionPixel, RGN_XOR);
                    DeleteObject(hRegion);
                    DeleteObject(hRegionPixel);
                    hRegion = hRegionDest;
                }
            }
        }

        SetWindowRgn(hWnd, hRegion, true);
        DeleteObject(hRegion);

        // Set the transparency
        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA);
        return true;
    }
#elif defined (SFML_SYSTEM_LINUX)
    #include <X11/Xatom.h>
    #include <X11/extensions/shape.h>

    bool setTransparent(Window wnd, const sf::Image& image, unsigned char alpha)
    {
        const sf::Uint8* pixelData = image.getPixelsPtr();
        Display* display = XOpenDisplay(NULL);

        // Set the transparency
        if (alpha != 255)
        {
            unsigned long opacity = (0xffffffff / 0xff) * alpha;
            Atom property = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", false);
            if (property != None)
                XChangeProperty(display, wnd, property, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&opacity, 1);
        }

        // Try to set the window shape
        int event_base;
        int error_base;
        if (XShapeQueryExtension(display, &event_base, &error_base))
        {
            Pixmap pixmap = XCreatePixmap(display, wnd, image.getSize().x, image.getSize().y, 1);
            GC gc = XCreateGC(display, pixmap, 0, NULL);

            XSetForeground(display, gc, 1);
            XFillRectangle(display, pixmap, gc, 0, 0, image.getSize().x, image.getSize().y);
            XSetForeground(display, gc, 0);

            for (unsigned int y = 0; y < image.getSize().y; y++)
            {
                for (unsigned int x = 0; x < image.getSize().x; x++)
                {
                    if (pixelData[y * image.getSize().x * 4 + x * 4 + 3] == 0)
                        XFillRectangle(display, pixmap, gc, x, y, 1, 1);
                }
            }

            XShapeCombineMask(display, wnd, ShapeBounding, 0, 0, pixmap, ShapeSet);
            XFreePixmap(display, pixmap);
            XFreeGC(display, gc);
            XFlush(display);
            return true;
        }
        else // Shape extension is not supported, window will not be of requested shape
        {
            XFlush(display);
            return false;
        }
    }

    #undef None // None conflicts with SFML
#endif

int main()
{
    // Change this to the wanted transparency
    const unsigned char opacity = 150;

    // Load an image with transparent parts that will define the shape of the window
    sf::Image backgroundImage;
    backgroundImage.loadFromFile("image.png");

    // Create the window and center it on the screen
    sf::RenderWindow window(sf::VideoMode(backgroundImage.getSize().x, backgroundImage.getSize().y, 32), "Transparent Window", sf::Style::None);
    window.setPosition(sf::Vector2i((sf::VideoMode::getDesktopMode().width - backgroundImage.getSize().x) / 2,
                                    (sf::VideoMode::getDesktopMode().height - backgroundImage.getSize().y) / 2));

    if (!setTransparent(window.getSystemHandle(), backgroundImage, opacity))
    {
        // You can execute some code here on unsupported OS or when linux display doesn't support shape extention
    }

    // We will also draw the image on the window instead of just showing an empty window with the wanted shape
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    backgroundTexture.loadFromImage(backgroundImage);
    backgroundSprite.setTexture(backgroundTexture);

    // Main loop to display the image while the window is open (pressing the escape key to close the window)
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                window.close();
        }

        window.clear();
        window.draw(backgroundSprite);
        window.display();
    }

    return 0;
}

