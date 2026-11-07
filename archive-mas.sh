#!/bin/sh
# Build a Mac App Store archive of KoboDeluxe.app (Phase 2.2).
#
# Usage:  ./archive-mas.sh <APPLE_TEAM_ID>
#
# Prereqs: Xcode with a signed-in Apple Developer account that has a
# "Mac App Store" distribution capability (automatic signing is fine).
#
# Output:  build/KoboDeluxe-mas.xcarchive  (and, with -upload, the
#          exported .ipa in build/export/).
set -eu

TEAM_ID="${1:?usage: $0 <APPLE_TEAM_ID>}"
cd "$(dirname "$0")"

rm -rf build-xcode-mas
cmake -B build-xcode-mas -G Xcode -DKOBO_MAS=ON

xcodebuild \
  -project build-xcode-mas/KoboDeluxe.xcodeproj \
  -scheme KoboDeluxe \
  -configuration Release \
  -destination "platform=macOS" \
  CODE_SIGN_STYLE=Automatic \
  DEVELOPMENT_TEAM="$TEAM_ID" \
  archive \
  -archivePath build/KoboDeluxe-mas.xcarchive

cat > build/export-options-mas.plist <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>method</key>
	<string>app-store</string>
	<key>teamID</key>
	<string>$TEAM_ID</string>
	<key>uploadSymbols</key>
	<true/>
</dict>
</plist>
EOF

# Local verification of the archived, sandboxed build:
#   open build/KoboDeluxe-mas.xcarchive/Products/Applications/KoboDeluxe.app
xcodebuild -exportArchive \
  -archivePath build/KoboDeluxe-mas.xcarchive \
  -exportOptionsPlist build/export-options-mas.plist \
  -exportPath build/export

echo "Archive:  build/KoboDeluxe-mas.xcarchive"
echo "Export:   build/export/ (drag to Transporter / App Store Connect)"
