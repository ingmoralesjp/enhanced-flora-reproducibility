# Remote publication checklist

1. Connect the intended GitHub account. Do not paste access tokens into chat,
   repository files or logs.
2. Create `enhanced-flora-reproducibility` privately unless public visibility
   is explicitly selected. Never overwrite an existing remote repository.
3. Push this prepared history and verify the remote tree/commit and access.
4. Run `python3 tools/check_publication_contents.py` before uploading. Inspect
   nested archives as well as visible paths. Keep manuscript drafts and paper
   projects outside the repository and release assets. Review separately listed
   large assets before uploading; a known hash does not prove availability.
5. Run the integrity/data/propagation checks from a new clone. Record the
   actual commit, tag, release URLs and asset hashes.
6. Create a versioned release after verification; use the artifact version,
   not the historical simulator's rc11 label as an ambiguous release identity.
7. Confirm reviewer access. If desired, archive that release in a repository
   such as Zenodo to obtain an archival identifier; no DOI exists until minted.
8. Replace the manuscript's availability statement with the actual repository,
   version/release and explicit boundaries of the shared files. Never claim
   the native compendium, raw outputs or final LaTeX is included if it is not.

The private local publication receipts and the original reviewer correspondence
are intentionally not copied into this repository. Historical evidence is
preserved separately from the publication working tree.
