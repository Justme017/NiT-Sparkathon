# NiT Sparkathon — Team Project Submission Guide

Welcome teams! This document explains how to upload your project into the NiT Sparkathon main repository when you are NOT a collaborator. The required workflow is: fork the event repository, add your project under the `Team Projects/` folder in your fork, push your branch, and open a Pull Request (PR) back to the main event repo.

Follow the sections below carefully. Example git commands use PowerShell (Windows) — replace placeholders (ALL CAPS) with your values.

## Quick summary (short)
- Fork: https://github.com/Justme017/NiT-Sparkathon
- Clone your fork locally
- Create a topic branch: `team/TEAMNAME-submission`
- Add your project into `Team Projects/TEAM_NAME/`
- Commit, push branch to your fork
- Open a Pull Request (base: `Justme017:main`, head: `your-username:team/TEAMNAME-submission`)

## Required repository layout for your submission
Create a single folder under `Team Projects/` named using the pattern:

Team_<TeamName>

Example:

Team_Alpha

Inside that folder include at minimum:
- `README.md` — short project description, how to build/run, Problem statement, Pitch Presentation, Videos and images , tech stack, and contact (team members) info.
- `LICENSE` — recommended (MIT/Apache2), or state license in README.
- `Platform io Files/` — source code.
- `docs/` — short documentation, architecture, or design notes (optional but recommended).
- `demo/` or `media/` — screenshots, short video link (YouTube/Vimeo/drive), or GIFs.
- `build_instructions.md` — exact steps to build/run (OS, prerequisites, commands).

Keep the folder self-contained and small. If your project has large binaries (>100 MB) use external hosting (Google Drive, Dropbox, GitHub Releases) and link from your README — do not commit large blobs directly (GitHub limits apply).

## Detailed step-by-step: Fork → PR (with PowerShell examples)

1) Fork the main event repository

- In your browser, open: https://github.com/Justme017/NiT-Sparkathon
- Click the "Fork" button (top-right) and fork to your GitHub account.

2) Clone your fork locally

Replace `<YOUR_USERNAME>` with your GitHub handle and `<FORK_URL>` if using SSH/HTTPS.

```powershell
# HTTPS example
git clone https://github.com/<YOUR_USERNAME>/NiT-Sparkathon.git

# OR SSH example
git clone git@github.com:<YOUR_USERNAME>/NiT-Sparkathon.git
cd NiT-Sparkathon
```

3) Add upstream remote and sync

```powershell
git remote add upstream https://github.com/Justme017/NiT-Sparkathon.git
git fetch upstream
# Keep your local main up-to-date
git checkout main
git merge upstream/main
git push origin main
```

4) Create a new feature branch for your submission

Use a clear branch name: `team/<TeamName>-submission` or `submission/<TeamName>`

```powershell
git checkout -b team/Team_SparkRockets_07-submission
```

5) Add your project files under `Team Projects/`

Create your team folder and add files. Example using PowerShell:

```powershell
mkdir "Team Projects\Team_Alpha"
cd "Team Projects\Team_Alpha"
# Copy your project files here, or create files directly
# Example: create a README
"# Team Alpha" | Out-File -FilePath README.md -Encoding utf8
cd ..\..

# Stage, commit
git add "Team Projects/Team_Alpha"
git commit -m "Team_Alpha: add initial project submission"
```

6) Push your branch to your fork

```powershell
git push -u origin team/Team_Alpha-submission
```

7) Open a Pull Request (PR)

- In your fork on GitHub you'll see a prompt to open a Pull Request after pushing. Or go to the upstream repo and click "New Pull Request" then choose:
	- base repository: `Justme017/NiT-Sparkathon`, branch: `main`
	- head repository: `<YOUR_USERNAME>/NiT-Sparkathon`, branch: `team/Team_Alpha-submission`
- Title your PR like: `[Team_Alpha] - Project Submission`
- In the PR description include the PR template below (fill in):

PR Description template (paste & edit):

```
Team name: Team Alpha
Contact (members): Name — email/phone/GitHub profile
Short description: One-paragraph summary of the project
How to run: Brief summary; point to build_instructions.md
Demo link(s): URL(s) to video or live demo
Files included: list of important files/folders
Any special notes: external files, big data, extra permissions
```

8) Update your PR (if requested by reviewers)

If maintainers request changes, make the edits locally on the same branch and push — the PR will update automatically:

```powershell
# Make changes locally, then:
git add .
git commit -m "Address reviewer feedback"
git push
```

9) Keep your fork in sync (optional, recommended if long-running)

```powershell
git fetch upstream
git checkout main
git merge upstream/main
git push origin main
# If your PR branch needs rebasing, rebase onto updated main and force-push:
git checkout team/Team_Alpha-submission
git rebase main
git push --force-with-lease
```

## PR checklist (what reviewers will look for)
- Team folder present under `Team Projects/Team_<Name>_<ID>`
- `README.md` with project summary, contact info, and run steps
- Clear build/run instructions (OS and dependency list)
- Demo link(s) (video or hosted app) or screenshots
- No secrets or API keys committed
- Repo size reasonable (no files >100 MB). If yes, provide external link or use Git LFS and note it in README.
- License chosen or stated

## Naming conventions & tips
- Use ASCII characters in folder/branch names (avoid spaces and special characters). Use underscores (_) or dashes (-).
- Branch name examples: `team/<teamname>-submission`, `submission/<teamname>-v1`.
- Keep commit messages clear and small (one change per commit where possible).

## Common issues & troubleshooting
- Large files blocked by GitHub: move to external hosting and include link.
- Forgot to include a file: add it on the same branch and push — PR updates automatically.
- Need to make a hotfix after PR merged: open a new PR with patch branch.
- Accidental secrets committed: rotate credentials immediately and notify maintainers.

## Security & privacy
- Do not commit passwords, API keys, or personal data. Use environment variables or a secure secrets store.

## Contact / Support
If you hit any problems with the fork/PR workflow, reach out to the event maintainers (organizer GitHub user: `Justme017`) or contact the event help channel/email provided in the event materials.

---
Don't forget to star this repository !!
Thank you for participating — good luck and happy building!

