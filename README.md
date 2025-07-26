<!-- This file was created using claude.ai/chat, and edited -->

# Low Battery Alert (lbagtk)

A lightweight GTK4 application that monitors battery levels and displays customizable alerts when your battery is running low. Perfect for Linux users who want elegant, non-intrusive battery warnings.

## ✨ Features

- **Real-time battery monitoring** - Continuously tracks battery percentage and charging status
- **Dual-threshold system** - Configurable low and risky battery levels
- **Customizable actions** - Execute any command (hibernate, suspend, etc.) via secondary button
- **CSS styling support** - Fully customizable appearance with CSS
- **Keyboard shortcuts** - ESC key to dismiss alerts
- **Daemon mode** - Run silently in the background
- **Auto-hide logic** - Smart window visibility based on battery state and user interaction

## 🚀 Quick Start

### Prerequisites

- GTK4 development libraries
- GLib development libraries
- A C compiler (clang recommended)
- Linux with `/sys/class/power_supply/` battery interface

### Installation

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install libgtk-4-dev libglib2.0-dev build-essential

# Install dependencies (Arch Linux)
sudo pacman -S gtk4 glib2 base-devel

# Compile
make

# Install to /usr/local/bin/lbagtk (optional)
make install
```

### Basic Usage

```bash
# Run with defaults
./lbagtk

# Run as daemon
./lbagtk --daemon

# Custom thresholds
./lbagtk --low 25 --risky 15

# Custom styles
./lbagtk --styles ~/.config/myapp/custom.css
```

## ⚙️ Configuration

### Command Line Options

| Option | Short | Description | Default |
|--------|-------|-------------|---------|
| `--daemon` | `-D` | Run as background process | No |
| `--styles FILE` | `-s` | Path to CSS styles file | `~/.config/gall/lbagtk.css` |
| `--low LEVEL` | `-l` | Low battery warning level | 20% |
| `--risky LEVEL` | `-r` | Critical level (enables action button) | 10% |
| `--btn STR` | `-b` | Secondary button text | "Hibernate" |
| `--btn-cmd CMD` | `-B` | Command to execute on button press | `systemctl hibernate` |

### CSS Customization

The application supports CSS theming. With gtk4, `:root { ... }` and `var(--name)` are available.
Create a CSS file (default: `~/.config/gall/lbagtk.css`) with these classes:

```css
/* Main window */
.battery-alert-window {
    background: rgba(40, 40, 40, 0.95);
    border-radius: 12px;
    border: 2px solid #ff6b6b;
}

/* Header text */
.battery-alert-header {
    color: #ff6b6b;
    font-weight: bold;
}

/* Battery info text */
.battery-alert-info {
    color: #ffffff;
}

/* Action button (hibernate/suspend) */
.battery-alert-action-btn {
    background: #ff6b6b;
    color: white;
    border-radius: 6px;
    padding: 8px 16px;
}

/* Dismiss button */
.battery-alert-close-btn {
    background: #444444;
    color: white;
    border-radius: 6px;
    padding: 8px 16px;
}

/* Container styling */
.battery-alert-main-container {
    padding: 20px;
}

.battery-alert-label-container {
    margin-bottom: 15px;
}

.battery-alert-button-container {
    margin-top: 10px;
}
```

## 🔧 How It Works

### Battery Detection
The application automatically scans `/sys/class/power_supply/BAT0` through `/sys/class/power_supply/BAT9` to find your battery interface, reading from:
- `capacity` file for battery percentage
- `status` file for charging state

### Alert Logic
- **Above low threshold + charging**: Window hidden
- **Below low threshold**: Warning appears (secondary button inactive, visible)
- **Below risky threshold**: Window reappears, action button becomes active
- **User dismissal**: Window hides temporarily (low ➜ risk) or permanently after second dismiss

### Process Management
The secondary action button uses `fork()` and `execl()` to launch commands without blocking the main application

## 🐛 Troubleshooting

### No battery detected
- Ensure `/sys/class/power_supply/BAT*` directories exist
- Check battery numbering (some systems use BAT1, BAT2, etc.)
- Verify read permissions on battery files

### GTK errors
- Confirm GTK4 is properly installed
- Check if display server is running (X11/Wayland)
- Try running without daemon mode first

### CSS not loading
- Verify file path and permissions
- Use absolute paths instead of `~` expansion
- Try running without daemon mode for CSS error logs

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Free and open source software that you can use, modify, and distribute! 🫠

---

Built with 💜 for Linux
