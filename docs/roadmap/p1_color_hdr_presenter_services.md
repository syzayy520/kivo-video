# P1 Color, HDR, And Presenter Services

These fake services turn existing contracts into dedicated P1 generation paths outside the large fake session fixture.

## Services

- ColorScienceCore: `build_fake_color_output_matrix`.
- WindowsHdrCore: `build_fake_windows_hdr_output_contract`.
- VideoRenderCore: `build_fake_video_presenter_contract`.

## Verified Chain

ComplianceGateResult feeds ColorOutputMatrix. ColorOutputMatrix feeds WindowsHdrOutputContract. WindowsHdrOutputContract and ColorOutputMatrix feed VideoPresenterContract.

## Rules

- HDR/DV claim level comes from ComplianceGateResult.
- Windows HDR output references ColorOutputMatrix.
- Presenter references both HDR output and ColorOutputMatrix.
- 23.976/24p pacing has an explicit frame pacing decision.

