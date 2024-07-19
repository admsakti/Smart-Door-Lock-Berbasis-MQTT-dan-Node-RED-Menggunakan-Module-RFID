-- phpMyAdmin SQL Dump
-- version 5.2.0
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Generation Time: Jun 10, 2024 at 05:15 AM
-- Server version: 10.4.27-MariaDB
-- PHP Version: 8.2.0

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `smart_door_lock`
--

-- --------------------------------------------------------

--
-- Table structure for table `history`
--

CREATE TABLE `history` (
  `id` int(11) NOT NULL,
  `rfid` varchar(32) NOT NULL,
  `nama` varchar(32) DEFAULT NULL,
  `akses` timestamp NOT NULL DEFAULT current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `history`
--

INSERT INTO `history` (`id`, `rfid`, `nama`, `akses`) VALUES
(96, '149, 108, 62, 67', NULL, '2024-06-09 07:04:44'),
(97, '149, 108, 62, 67', NULL, '2024-06-09 07:05:04'),
(98, '149, 108, 62, 67', NULL, '2024-06-09 07:05:31'),
(99, '149, 108, 62, 67', NULL, '2024-06-09 07:05:53'),
(100, '149, 108, 62, 67', NULL, '2024-06-09 07:06:17'),
(101, '149, 108, 62, 67', NULL, '2024-06-09 07:06:39'),
(102, '149, 108, 62, 67', NULL, '2024-06-09 07:06:55'),
(103, '163, 107, 56, 10', '163, 107, 56, 10', '2024-06-09 07:09:43'),
(104, '163, 107, 56, 10', '163, 107, 56, 10', '2024-06-09 07:29:14'),
(105, '149, 108, 62, 67', '149, 108, 62, 67', '2024-06-09 07:34:27'),
(106, '163, 107, 56, 10', '163, 107, 56, 10', '2024-06-09 07:36:32'),
(107, '149, 108, 62, 67', '149, 108, 62, 67', '2024-06-09 07:36:55');

-- --------------------------------------------------------

--
-- Table structure for table `user`
--

CREATE TABLE `user` (
  `rfid` varchar(32) NOT NULL,
  `nama` varchar(32) DEFAULT NULL,
  `sign_in` timestamp NOT NULL DEFAULT current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `user`
--

INSERT INTO `user` (`rfid`, `nama`, `sign_in`) VALUES
('149, 108, 62, 67', 'Ayah', '2024-06-09 07:33:43'),
('163, 107, 56, 10', 'Kakak', '2024-06-09 07:08:02');

--
-- Indexes for dumped tables
--

--
-- Indexes for table `history`
--
ALTER TABLE `history`
  ADD PRIMARY KEY (`id`),
  ADD KEY `nama` (`nama`);

--
-- Indexes for table `user`
--
ALTER TABLE `user`
  ADD PRIMARY KEY (`rfid`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `history`
--
ALTER TABLE `history`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=108;

--
-- Constraints for dumped tables
--

--
-- Constraints for table `history`
--
ALTER TABLE `history`
  ADD CONSTRAINT `history_ibfk_1` FOREIGN KEY (`nama`) REFERENCES `user` (`rfid`) ON DELETE SET NULL ON UPDATE CASCADE;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
