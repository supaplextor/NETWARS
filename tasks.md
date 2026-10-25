# Outstanding tasks

This is the canonical backlog for unfinished work explicitly identified in the
repository. All items below are carried over from the **Known gaps** section of
`netwars-legacy-stub/README.md`.

## Legacy game implementation

- [ ] **Implement the polygon renderer** — Reconstruct the wireframe ship-shape
  drawing routines, including EGA planar drawing and the VGA mode 13h blitter,
  from executable offsets `0x16000–0x1AFFF`.
- [ ] **Implement the Sound Blaster driver** — Reconstruct the DSP command
  sequences from offsets `0x20000–0x229FF`.
- [ ] **Implement IPX packet send and receive** — Reconstruct the low-level
  ECB/ESR structures and packet handling from offsets `0x25A00–0x2A85F`.
- [ ] **Implement the shape editor** — Reconstruct the interactive polygon
  editing tool from offsets `0x13000–0x15FFF`, including mouse-driver support.
- [ ] **Implement the star-field engine** — Reconstruct the parallax-scrolling
  star renderer using the sine/cosine lookup tables at `DS:0x260B8`.
