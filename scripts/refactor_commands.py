import os
import re

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original_content = content

    # Replace HWND with yamy::platform::WindowHandle
    content = re.sub(r'\bHWND\b', 'yamy::platform::WindowHandle', content)

    # Replace RECT with yamy::platform::Rect
    content = re.sub(r'\bRECT\b', 'yamy::platform::Rect', content)

    # Replace POINT with yamy::platform::Point
    content = re.sub(r'\bPOINT\b', 'yamy::platform::Point', content)

    # Replace i_engine->m_windowSystem with i_engine->getWindowSystem()
    content = content.replace('i_engine->m_windowSystem', 'i_engine->getWindowSystem()')

    # Update getSuitableWindow call
    # Old: Engine::getSuitableWindow(i_param, &hwnd)
    # New: Engine::getSuitableWindow(i_engine->getWindowSystem(), i_param, &hwnd)
    content = re.sub(r'Engine::getSuitableWindow\s*\(\s*i_param', 'Engine::getSuitableWindow(i_engine->getWindowSystem(), i_param', content)

    # Update getSuitableMdiWindow call
    # Old call logic handled implicitly because m_windowSystem was replaced by getWindowSystem()

    # Remove old casts
    content = content.replace('(WindowSystem::WindowHandle)', '')
    content = content.replace('static_cast<WindowSystem::WindowHandle>', '')

    if content != original_content:
        print(f"Updating {filepath}")
        with open(filepath, 'w') as f:
            f.write(content)

def main():
    root_dir = 'src/core/commands'
    for filename in os.listdir(root_dir):
        if filename.endswith('.cpp') or filename.endswith('.h'):
            process_file(os.path.join(root_dir, filename))

if __name__ == '__main__':
    main()
