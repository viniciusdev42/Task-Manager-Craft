# Task-Manager-Craft

## Visão Geral

### Descrição

O **Task Manager Craft** é um gerenciador de tarefas desenvolvido em C++ que serve como aplicação e laboratório de estruturas de dados. Nele,cada estrutura de dados existe para resolver uma necessidade concreta do sistema. Cada estrutura possuí a sua própria implementação,testes unitários e testes de carga.

Por fora,o Task Manager Craft é um gerenciador de tarefas: o usuário cria tarefas com título,descrição,prioridade e prazo e marcação(concluída,em andamento,em espera,cancelada). Por dentro,o Task Manager Craft é um projeto de engenharia de software e de algoritmos: todo o armazenamento,indexação,busca e persistência são implementados do zero,sem delegar às estruturas de dados e bibliotecas padrão.

O Task Manager Craft utiliza as seguintes estruturas de dados:

1.`Hash Table`: Permite o **acesso direto** à tarefa pelo ID
2.`Lista de Prioridades (Heap)`: Permite o **Agendamento de Tarefas**
3.`Árvore AVL`: Permite **índices ordenados** (título,prazo,prioridade)
4.`Árvore TRIE`: Permite a busca por **prefixo** e **autocomplete**
5.`Processamento de Cadeia de Caracteres`: Permite a **normalização,a tokenização e a busca textual**.
6.`Árvore B+ & Pager`: Permite a **persistência** e a **indexação em disco**
7.`Grafos`: Permite gerenciar a **dependência entre tarefas**,a **detecção de ciclos** e a **ordenação topológica**
8.`Busca Digital`: Permite a busca baseada em **dígitos** (Digital Search Tree,Radix,Patricia)

### Arquitetura

```text
┌──────────────────────────────┐
│             GUI              │   Dashboard, tarefas, busca, calendário,
│                              │   dependências, configurações
├──────────────────────────────┤
│         Application          │   TaskService, SearchService,
│                              │   Scheduler, DependencyService
├──────────────────────────────┤
│            Domain            │   Task, Project, Dependency
├──────────────────────────────┤
│   Data Structures / Storage  │   Hash, AVL,B+-Tree, TRIE, Graph,
│                              │   Priority Queue, Digital Search,
│                              │   Pager, Serialização, Arquivo
└──────────────────────────────┘
                │
                ▼
              Disco
```

### Estrutura

```Text
TaskManager/
│
├── CMakeLists.txt
│
├── include/
│   ├── domain/
│   │   ├── task.hpp
│   │   ├── project.hpp
│   │   └── dependency.hpp
│   │
│   ├── application/
│   │   ├── task_service.hpp
│   │   ├── search_service.hpp
│   │   ├── scheduler.hpp
│   │   └── dependency_service.hpp
│   │
│   ├── structures/
│   │   ├── avl.hpp
│   │   ├── rb_tree.hpp
│   │   ├── b_tree.hpp
│   │   ├── trie.hpp
│   │   ├── hash_table.hpp
│   │   ├── priority_queue.hpp
│   │   ├── graph.hpp
│   │   └── digital_search.hpp
│   │
│   ├── storage/
│   │   ├── pager.hpp
│   │   ├── serializer.hpp
│   │   └── repository.hpp
│   │
│   └── gui/
│       ├── main_window.hpp
│       ├── task_view.hpp
│       ├── search_view.hpp
│       ├── graph_view.hpp
│       └── structure_view.hpp
│
├── src/
│   ├── domain/
│   ├── application/
│   ├── structures/
│   ├── storage/
│   └── gui/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   └── gui/
```

## Sobre o Projeto

O Task Manager Craft é desenvolvido utilizando IA Generativa (Claude e ChatGPT) para **programação,refatoração,testes,planejamento e sugestões**.

As principais decisões do projeto são tomadas com base nas **recomendações dos modelos**. Não estão incluídos modelos agênticos (Codex & Cowork) devido a questões de orçamento.
