"""R2 layout captures isolated from prior evidence; reuses navigation helpers."""
import detail_navigation_device as navigation

navigation.OUT = navigation.device.ROOT / "docs/evidence/p240/detail-layout"
navigation.device.OUT = navigation.OUT

if __name__ == "__main__":
    navigation.main()
