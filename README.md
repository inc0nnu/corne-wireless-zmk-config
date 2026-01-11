# Development reference guide

This guide reflects my workflow, which keeps upstream separate, keeps main stable, and keeps feature branches linear.

---

# 🧭 **1. Getting ready to start working on a keymap**  
*(Sync upstream → update your mirror → update main → create feature branch)*

```bash
# Make sure you're on main
git checkout main

# Fetch latest changes from original repo
git fetch upstream

# Update your local upstream mirror
git checkout upstream-sync
git merge upstream/master

# Bring upstream changes into your main branch
git checkout main
git merge upstream-sync

# Create a new feature branch for your keymap work
git checkout -b feature/my-keymap
```

---

# 🛠️ **2. Development iterations with commits**  
*(Edit → stage → commit → push)*

```bash
# After editing your keymap
git add path/to/my.keymap

# Commit your changes
git commit -m "Describe what changed"

# Push your feature branch (first time)
git push -u origin feature/my-keymap

# Later pushes
git push
```

---

# 🔁 **2b. (Optional but recommended) Rebase your feature branch onto main**  
*(Keeps your feature branch clean before merging)*

Only do this **on your feature branch**, never on `main`.

```bash
git checkout feature/my-keymap
git rebase main
```

This rewrites **your feature branch**, not `main`.

---

# 🌳 **3. Merging tested development work into main**  
*(Once your feature branch is stable and tested)*

```bash
# Switch to main
git checkout main

# Merge your feature branch
git merge feature/my-keymap

# Push updated main to your fork
git push
```

Optional cleanup:

```bash
git branch -d feature/my-keymap
```

---

# 🗂️ **4. Storing an unstaged file as a reference keymap**

## **Option A — Local-only stash (not committed)**  
Perfect for temporary or personal reference snapshots.

```bash
git stash push -m "reference keymap before experiment"
```

Retrieve later:

```bash
git stash list
git stash show -p stash@{0}
git stash apply stash@{0}
```

## **Option B — Permanent archived branch**  
Ideal for long-term layout history.

```bash
git checkout -b archive/my-reference-keymap
git add path/to/my.keymap
git commit -m "Archive reference keymap"
git push -u origin archive/my-reference-keymap
```

---

# 🎯 **Summary of the rebase rule**

- ✔ Rebase **feature branches**  
- ✔ Merge **feature → main**  
- ✔ Merge **upstream → main**  
- ❌ Never rebase `main`  
- ❌ Never rebase upstream  

