{ pkgs }: {
    deps = [
        pkgs.go
        pkgs.nodejs
        pkgs.nodePackages_latest.pnpm
    ];
}
