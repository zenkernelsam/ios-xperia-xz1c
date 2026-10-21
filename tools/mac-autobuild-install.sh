#!/bin/bash
# mac-autobuild-install.sh — install the xnu build slave as a launchd agent
# so it survives reboots and starts at login. Run on the Mac once:
#   bash tools/mac-autobuild-install.sh
set -eu

REPO="${XNU_REPO:-$HOME/Desktop/ios-xperia-xz1c}"
LABEL="com.xz1c.xnu-autobuild"
PLIST="$HOME/Library/LaunchAgents/$LABEL.plist"

mkdir -p "$HOME/Library/LaunchAgents"
cat >"$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>Label</key>
	<string>$LABEL</string>
	<key>ProgramArguments</key>
	<array>
		<string>/bin/bash</string>
		<string>$REPO/tools/mac-autobuild.sh</string>
	</array>
	<key>RunAtLoad</key>
	<true/>
	<key>KeepAlive</key>
	<true/>
	<key>ThrottleInterval</key>
	<integer>30</integer>
	<key>StandardOutPath</key>
	<string>$HOME/xnu-autobuild.out.log</string>
	<key>StandardErrorPath</key>
	<string>$HOME/xnu-autobuild.out.log</string>
</dict>
</plist>
EOF

launchctl unload "$PLIST" 2>/dev/null || true
launchctl load -w "$PLIST"
echo "installed $LABEL — log: ~/xnu-autobuild.log"
launchctl list | grep xz1c || true
