"""R2 playback entry captures, isolated from earlier navigation evidence."""
import detail_navigation_device as navigation

navigation.OUT = navigation.device.ROOT / "docs/evidence/p240/detail-playback"
navigation.device.OUT = navigation.OUT

if __name__ == "__main__":
    navigation.main()
