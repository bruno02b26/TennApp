# TennApp

TennApp is a tennis match management system written in C++. This application enables users to schedule matches and record match outcomes.

## Key Features

- **Player Management**: Add, view, and manage player profiles.
- **Match Management**: Add, start, suspend, resume and finish matches. The application tracks various match states and ensures the integrity of match data. These types are handled using the State design pattern.
- **Score Tracking**: Record points during a match, with automated score calculation following tennis rules.
- **Comprehensive Match Statistics**: View detailed match information including set-by-set scores, game points, and tiebreak results.
- **Real-time Duration Tracking**: Track the duration of matches in real-time, updating the database with the exact time elapsed.
- **Database Integration**: Uses PostgreSQL to store all match-related data, including players, match states, scores, and durations.
- **Dockerized Database Setup**: The PostgreSQL database is set up and managed using Docker, ensuring a consistent and isolated environment for development and deployment.

## Technologies Used

- **C++**: The core application logic, including player and match management, is implemented in C++.
- **PostgreSQL**: All data related to players and matches is stored in a PostgreSQL database.
- **Docker**: Docker is used to set up and run the PostgreSQL database in a containerized environment, simplifying the deployment process and ensuring consistency across different environments.

## Dependencies

This project requires the following dependencies:

- [tabulate](https://github.com/p-ranav/tabulate)
- [libpqxx](https://github.com/jtv/libpqxx)

## Demo

Check out [YouTube video](https://www.youtube.com/watch?v=NVj8IQSlTo8) for a brief overview of the application and its features.
