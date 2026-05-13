# GITHUB_AND_CODEX_SETUP.md — как начать работу с GitHub и Codex

## 1. Создать локальный репозиторий

```bash
mkdir petri-core
cd petri-core
git init
```

Скопируй в корень репозитория файлы:

- `README.md`
- `AGENTS.md`
- `PROJECT.md`
- `PLAN.md`
- `API_CONTRACT.md`
- `CODEX_INIT_PROMPT.md`
- папку `examples/`

Затем:

```bash
git add .
git commit -m "docs: add initial project context for codex"
```

## 2. Создать репозиторий на GitHub

На GitHub создай новый репозиторий, например `petri-core`.

После создания GitHub покажет команды. Обычно они такие:

```bash
git remote add origin git@github.com:<username>/petri-core.git
git branch -M main
git push -u origin main
```

Если используешь HTTPS:

```bash
git remote add origin https://github.com/<username>/petri-core.git
git branch -M main
git push -u origin main
```

## 3. Подключить репозиторий к ChatGPT/Codex

В ChatGPT:

1. Открой `Settings`.
2. Перейди в `Apps`.
3. Найди `GitHub`.
4. Нажми `Connect`.
5. На стороне GitHub разреши доступ к нужному репозиторию.
6. Выбери `petri-core` в списке доступных репозиториев.

Если репозиторий не появился сразу, подожди несколько минут или проверь настройки доступа GitHub App.

## 4. Запустить Codex локально

Вариант через CLI:

```bash
npm install -g @openai/codex
cd petri-core
codex
```

После запуска выбери вход через ChatGPT и открой проектную папку.

Вариант через Codex App:

1. Установи Codex App.
2. Войди через ChatGPT.
3. Выбери папку `petri-core`.
4. Создай новый поток и вставь промпт из `CODEX_INIT_PROMPT.md`.

## 5. Рекомендуемый рабочий процесс

1. Создавай отдельную ветку под задачу:

```bash
git checkout -b feature/core-pn-first-slice
```

2. Дай Codex задачу из `CODEX_INIT_PROMPT.md`.
3. Проверь diff.
4. Запусти тесты локально.
5. Закоммить результат:

```bash
git add .
git commit -m "core_pn: add first simulation slice"
git push -u origin feature/core-pn-first-slice
```

6. Создай Pull Request на GitHub.

## 6. Что просить у Codex после первого среза

- Добавить Python-биндинги через `pybind11`.
- Добавить FastAPI-обёртку вокруг Python-модуля, если второй разработчик вызывает ядро по HTTP.
- Добавить визуализационно-удобный формат событий.
- Добавить CSV/JSON-экспорт метрик.
- Добавить выборщик алгоритмов.
