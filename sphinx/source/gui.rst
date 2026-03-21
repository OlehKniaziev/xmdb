GUI
===

The XMDB graphical interface is a web application that connects to a running XMDB server.
It provides a SQL query editor, a media gallery for visualising image data, and a database object browser.

.. contents:: Contents
   :local:
   :depth: 1

----

Technologies
------------

The GUI is built with the following stack:

* **React 19**
* **TypeScript 5.8**
* **Vite 6**
* **Zustand 5**
* **React Router 7**
* **Monaco Editor**
* **react-resizable-panels**
* **Lucide React**
* **ESLint**

Requirements
------------

* **Node.js** >= 18 (LTS recommended)
* **npm** (bundled with Node.js)
* A running XMDB server instance to connect to (see :doc:`server`)

Running the GUI
---------------

All commands are run from the ``gui/`` directory.

Install dependencies (first time only):

.. code-block:: shell

                npm install

Start the development server:

.. code-block:: shell

                npm run dev

The application is served at ``http://localhost:5173`` by default.

Other commands
--------------

Build for production:

.. code-block:: shell

                npm run build

Run the linter:

.. code-block:: shell

                npm run lint

Preview the production build locally:

.. code-block:: shell

                npm run preview

Project structure
-----------------

::

  gui/src/
    App.tsx              -- Root router and route definitions
    layouts/             -- Page shells (MainLayout with sidebar, toolbar, tab bar)
    components/          -- Feature components (QueryEditor, Sidebar, GalleryView, ObjectView, etc.)
    data/
      global-states.ts   -- Zustand stores (connection, multi-tab query state)
      objects.ts         -- Types matching the /get-db-objects API response
      query-response.ts  -- Types matching the /run-query API response
      util.ts            -- Helpers (file picking, hex conversion, image encoding)
    styles/              -- CSS stylesheets
    assets/              -- Icons and static assets
