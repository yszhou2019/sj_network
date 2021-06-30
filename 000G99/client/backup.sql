-- MySQL dump 10.13  Distrib 8.0.21, for Linux (x86_64)
--
-- Host: localhost    Database: db1752240
-- ------------------------------------------------------
-- Server version	8.0.21

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

USE db1752240;

--
-- Table structure for table `dir`
--

DROP TABLE IF EXISTS `dir`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `dir` (
  `dir_id` int NOT NULL AUTO_INCREMENT,
  `dir_path` varchar(1024) NOT NULL,
  `dir_uid` int NOT NULL,
  `dir_bindid` int NOT NULL COMMENT '绑定的服务器目录的id',
  PRIMARY KEY (`dir_id`),
  KEY `fk_iskf_2` (`dir_uid`),
  CONSTRAINT `fk_iskf_2` FOREIGN KEY (`dir_uid`) REFERENCES `user` (`user_id`)
) ENGINE=InnoDB AUTO_INCREMENT=20 DEFAULT CHARSET=gb2312;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `physical_file`
--

DROP TABLE IF EXISTS `physical_file`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `physical_file` (
  `pfile_id` int NOT NULL AUTO_INCREMENT,
  `pfile_md5` varchar(32) CHARACTER SET gb2312 COLLATE gb2312_chinese_ci NOT NULL,
  `pfile_refcnt` int NOT NULL DEFAULT '0',
  `pfile_name` varchar(100) CHARACTER SET gb2312 COLLATE gb2312_chinese_ci NOT NULL COMMENT '实际文件名',
  PRIMARY KEY (`pfile_id`)
) ENGINE=InnoDB DEFAULT CHARSET=gb2312;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `user`
--

DROP TABLE IF EXISTS `user`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `user` (
  `user_id` int NOT NULL AUTO_INCREMENT,
  `user_name` varchar(20) NOT NULL,
  `user_pwd` varchar(255) CHARACTER SET gb2312 COLLATE gb2312_chinese_ci NOT NULL,
  `user_bindid` int DEFAULT '0' COMMENT '0代表默认目录',
  PRIMARY KEY (`user_id`)
) ENGINE=InnoDB AUTO_INCREMENT=19 DEFAULT CHARSET=gb2312;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `user_session`
--

DROP TABLE IF EXISTS `user_session`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `user_session` (
  `session` varchar(32) NOT NULL COMMENT 'session',
  `session_uid` int NOT NULL COMMENT '用户号',
  KEY `fk_device_uid` (`session_uid`),
  CONSTRAINT `fk_device_uid` FOREIGN KEY (`session_uid`) REFERENCES `user` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=gb2312 COMMENT='这个表暂时用不到，将session字段合并到user表中';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `virtual_file`
--

DROP TABLE IF EXISTS `virtual_file`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `virtual_file` (
  `vfile_id` int NOT NULL AUTO_INCREMENT COMMENT '文件id',
  `vfile_dir_id` int NOT NULL COMMENT '路径名对应的父目录id',
  `vfile_name` varchar(255) CHARACTER SET gb2312 COLLATE gb2312_chinese_ci NOT NULL COMMENT '文件名',
  `vfile_size` bigint NOT NULL COMMENT '虚拟文件的大小（如果是目录则为0）',
  `vfile_md5` varchar(32) CHARACTER SET gb2312 COLLATE gb2312_chinese_ci NOT NULL COMMENT '目录文件md5为空',
  `vfile_mtime` int DEFAULT NULL COMMENT '服务器的最新变动时间',
  `vfile_chunks` mediumtext CHARACTER SET gb2312 COLLATE gb2312_chinese_ci COMMENT '经过序列化的chunks信息，最多保存1667万字节',
  `vfile_cnt` int DEFAULT '0',
  `vfile_total` int DEFAULT NULL,
  `vfile_complete` tinyint DEFAULT '0',
  PRIMARY KEY (`vfile_id`),
  KEY `fk_iskf_3` (`vfile_dir_id`),
  CONSTRAINT `fk_iskf_3` FOREIGN KEY (`vfile_dir_id`) REFERENCES `dir` (`dir_id`)
) ENGINE=InnoDB AUTO_INCREMENT=21 DEFAULT CHARSET=gb2312;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2021-06-30 16:56:00
