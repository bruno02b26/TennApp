CREATE SCHEMA public;

CREATE TYPE public.points AS ENUM (
    '0',
    '15',
    '30',
    '40',
    'A'
);

ALTER TYPE public.points OWNER TO postgres;

CREATE TABLE public.game_points (
    match_id integer NOT NULL,
    set_number integer NOT NULL,
    game_number integer NOT NULL,
    player1_points public.points NOT NULL,
    player2_points public.points NOT NULL
);

ALTER TABLE public.game_points OWNER TO postgres;

CREATE TABLE public.match_status (
    id integer NOT NULL,
    status character varying(20) NOT NULL
);

ALTER TABLE public.match_status OWNER TO postgres;

CREATE TABLE public.matches (
    id integer NOT NULL,
    status_id integer NOT NULL,
    player_id1 integer NOT NULL,
    player_id2 integer NOT NULL,
    winner_id integer,
    predicted_start_time timestamp without time zone NOT NULL,
    duration interval NOT NULL,
    no_sets integer NOT NULL,
    actual_start_time timestamp without time zone,
    CONSTRAINT matches_numberofsets_check CHECK (((no_sets >= 1) AND (no_sets <= 3)))
);

ALTER TABLE public.matches OWNER TO postgres;

CREATE SEQUENCE public.matches_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;

ALTER SEQUENCE public.matches_id_seq OWNER TO postgres;

ALTER SEQUENCE public.matches_id_seq OWNED BY public.matches.id;

CREATE TABLE public.matches_sets (
    match_id integer NOT NULL,
    set_number integer NOT NULL,
    games_won_player1 integer DEFAULT 0,
    games_won_player2 integer DEFAULT 0,
    is_tie_break boolean DEFAULT false,
    is_first_player_serving boolean DEFAULT false
);

ALTER TABLE public.matches_sets OWNER TO postgres;

CREATE TABLE public.players (
    id integer NOT NULL,
    first_name character varying(25) NOT NULL,
    last_name character varying(25) NOT NULL,
    matches_won integer DEFAULT 0,
    matches_lost integer DEFAULT 0
);

ALTER TABLE public.players OWNER TO postgres;

CREATE SEQUENCE public.players_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;

ALTER SEQUENCE public.players_id_seq OWNER TO postgres;

ALTER SEQUENCE public.players_id_seq OWNED BY public.players.id;

CREATE TABLE public.tie_break_type (
    type_id integer NOT NULL,
    min_points integer NOT NULL
);

ALTER TABLE public.tie_break_type OWNER TO postgres;

CREATE TABLE public.tie_breaks (
    tie_break_id integer NOT NULL,
    match_id integer NOT NULL,
    set_number integer NOT NULL,
    player1_score integer NOT NULL,
    player2_score integer NOT NULL,
    tie_break_type integer NOT NULL
);

ALTER TABLE public.tie_breaks OWNER TO postgres;

CREATE SEQUENCE public.tie_breaks_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;

ALTER SEQUENCE public.tie_breaks_id_seq OWNER TO postgres;

ALTER SEQUENCE public.tie_breaks_id_seq OWNED BY public.tie_breaks.tie_break_id;

ALTER TABLE ONLY public.matches ALTER COLUMN id SET DEFAULT nextval('public.matches_id_seq'::regclass);

ALTER TABLE ONLY public.players ALTER COLUMN id SET DEFAULT nextval('public.players_id_seq'::regclass);

ALTER TABLE ONLY public.tie_breaks ALTER COLUMN tie_break_id SET DEFAULT nextval('public.tie_breaks_id_seq'::regclass);

COPY public.match_status (id, status) FROM stdin;
1	Started
2	Suspended
3	Finished
4	Pending
5	Delayed
\.

COPY public.tie_break_type (type_id, min_points) FROM stdin;
1	7
2	10
\.

SELECT pg_catalog.setval('public.matches_id_seq', 1, false);

SELECT pg_catalog.setval('public.players_id_seq', 1, false);

SELECT pg_catalog.setval('public.tie_breaks_id_seq', 1, false);

ALTER TABLE public.game_points
    ADD CONSTRAINT game_points_game_number_check CHECK ((game_number > 0)) NOT VALID;

ALTER TABLE ONLY public.game_points
    ADD CONSTRAINT game_points_pkey PRIMARY KEY (match_id, set_number, game_number);

ALTER TABLE public.game_points
    ADD CONSTRAINT game_points_set_number_check CHECK ((set_number > 0)) NOT VALID;

ALTER TABLE ONLY public.match_status
    ADD CONSTRAINT match_status_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.match_status
    ADD CONSTRAINT match_status_status_key UNIQUE (status);

ALTER TABLE public.matches
    ADD CONSTRAINT matches_check CHECK (((winner_id = player_id1) OR (winner_id = player_id2))) NOT VALID;

ALTER TABLE ONLY public.matches
    ADD CONSTRAINT matches_pkey PRIMARY KEY (id);

ALTER TABLE public.matches_sets
    ADD CONSTRAINT matches_sets_games_won_player1_check CHECK ((games_won_player1 >= 0)) NOT VALID;

ALTER TABLE public.matches_sets
    ADD CONSTRAINT matches_sets_games_won_player2_check CHECK ((games_won_player2 >= 0)) NOT VALID;

ALTER TABLE ONLY public.matches_sets
    ADD CONSTRAINT matches_sets_pkey PRIMARY KEY (match_id, set_number);

ALTER TABLE public.players
    ADD CONSTRAINT players_matches_lost_check CHECK ((matches_lost >= 0)) NOT VALID;

ALTER TABLE public.players
    ADD CONSTRAINT players_matches_won_check CHECK ((matches_won >= 0)) NOT VALID;

ALTER TABLE ONLY public.players
    ADD CONSTRAINT players_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.tie_break_type
    ADD CONSTRAINT tie_break_type_pkey PRIMARY KEY (type_id);

ALTER TABLE ONLY public.tie_breaks
    ADD CONSTRAINT tie_breaks_pkey PRIMARY KEY (tie_break_id);

ALTER TABLE public.tie_breaks
    ADD CONSTRAINT tie_breaks_player1_score_check CHECK ((player1_score >= 0)) NOT VALID;

ALTER TABLE public.tie_breaks
    ADD CONSTRAINT tie_breaks_player2_score_check CHECK ((player2_score >= 0)) NOT VALID;

ALTER TABLE public.tie_breaks
    ADD CONSTRAINT tie_breaks_set_number_check CHECK ((set_number > 0)) NOT VALID;

ALTER TABLE ONLY public.game_points
    ADD CONSTRAINT game_points_matchid_fkey FOREIGN KEY (match_id) REFERENCES public.matches(id);

ALTER TABLE ONLY public.game_points
    ADD CONSTRAINT game_points_matchid_setnumber_fkey FOREIGN KEY (match_id, set_number) REFERENCES public.matches_sets(match_id, set_number);

ALTER TABLE ONLY public.matches
    ADD CONSTRAINT matches_playerid1_fkey FOREIGN KEY (player_id1) REFERENCES public.players(id);

ALTER TABLE ONLY public.matches
    ADD CONSTRAINT matches_playerid2_fkey FOREIGN KEY (player_id2) REFERENCES public.players(id);

ALTER TABLE ONLY public.matches_sets
    ADD CONSTRAINT matches_sets_matchid_fkey FOREIGN KEY (match_id) REFERENCES public.matches(id);

ALTER TABLE ONLY public.matches
    ADD CONSTRAINT matches_statusid_fkey FOREIGN KEY (status_id) REFERENCES public.match_status(id);

ALTER TABLE ONLY public.matches
    ADD CONSTRAINT matches_winnerid_fkey FOREIGN KEY (winner_id) REFERENCES public.players(id);

ALTER TABLE ONLY public.tie_breaks
    ADD CONSTRAINT tie_breaks_match_id_fkey FOREIGN KEY (match_id) REFERENCES public.matches(id);

ALTER TABLE ONLY public.tie_breaks
    ADD CONSTRAINT tie_breaks_tie_break_type_fkey FOREIGN KEY (tie_break_type) REFERENCES public.tie_break_type(type_id) NOT VALID;
