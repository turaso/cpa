#include <args/args.hh>
#include <cloud/cloud.hh>
#include <input/input.hh>

#include <map>

int upload(args::Parser& parser, const cloud::Cloud& cl) {
  // const auto album = parser.find("--album");
  // const auto path = parser.find("--path"); /// !! to be checked
  const auto validated =
      parser.require("--album").optional("--path").validate();
  if (!validated) {
    std::cerr << "Invalid usage or invalid parameters" << std::endl;
    return 1;
  }
  const auto album = parser.get("--album");
  const auto path = parser.get("--path"); /// !! to be checked

  if (album.empty()) {
    return 1;
  }
  if (
      !std::filesystem::is_directory(path)
      // || (std::filesystem::status(path).permissions()
          // != std::filesystem::perms::others_read)
      || !cl.upload(album, path)
  ) {
    return 1;
  }

  return 0;
}

int download(args::Parser& parser, const cloud::Cloud& cl) {
  // const auto album = parser.find("--album");
  // const auto path = parser.find("--path"); /// !! to be checked
  const auto validated =
      parser.require("--album").optional("--path").validate();
  if (!validated) {
    std::cerr << "Invalid usage or invalid parameters" << std::endl;
    return 1;
  }
  const auto album = parser.get("--album");
  const auto path = parser.get("--path"); /// !! to be checked

  if (album.empty()) {
    return 1;
  }
  if (
      !std::filesystem::is_directory(path)
      // || (std::filesystem::status(path).permissions()
          // != std::filesystem::perms::group_write)
      || !cl.download(album, path)) {
    return 1;
  }

  return 0;
}

int list(args::Parser& parser, const cloud::Cloud& cl) {
  // const auto album = parser.find("--album");
  const auto validated =
      parser.optional("--album").validate();
  if (!validated) {
    std::cerr << "Invalid usage or invalid parameters" << std::endl;
    return 1;
  }
  const auto album = parser.get("--album");

  if (album.empty()) {

    const auto albums = cl.albums();

    if (!albums.has_value()) {
      return 1;
    }

    if (albums.value().empty()) {
      std::cout << "empty list" << std::endl;
      return 0;
    }

    for (const auto& album : albums.value()) {
      std::cout << album << std::endl;
    }

  } else {

    const auto photos = cl.get(album);

    if (!photos.has_value()) {
      return 1;
    }

    if (photos.value().empty()) {
      std::cout << "empty list" << std::endl;
      return 0;
    }

    for (const auto& photo : photos.value()) {
      std::cout << photo << std::endl;
    }

  }

  return 0;
}

int del(args::Parser& parser, const cloud::Cloud& cl) {
  // const auto album = parser.find("--album");
  // const auto photo = parser.find("--photo"); /// !! to be checked
  const auto validated =
      parser.require("--album").optional("--photo").validate();
  if (!validated) {
    std::cerr << "Invalid usage or invalid parameters" << std::endl;
    return 1;
  }
  const auto album = parser.get("--album");
  const auto photo = parser.get("--photo"); /// !! to be checked

  if (album.empty()) {
    return 1;
  }
  if (photo.empty()) {
    if (!cl.del(album)) {
      return 1;
    }
  } else {
    if (!cl.del(album, photo)) {
      return 1;
    }
  }

  return 0;
}

int mksite(const cloud::Cloud& cl) {
  const auto url = cl.mksite();

  if (url.empty()) {
    return 1;
  }

  std::cout << url << std::endl;
  return 0;
}

int init(cloud::Cloud& cl) {
  const auto keyId = input::read("Enter key id: ");
  const auto key = input::read("Enter key: ");
  const auto bucket = input::read("Enter bucket name: ");

  if (!cl.configure(keyId, key, bucket)) {
    return 1;
  }

  if (!cl.init()) {
    return 1;
  }

  return 0;
}

int main(int argc, char** argv) {
  args::Parser parser(argc, argv);
  cloud::Cloud cl;
  enum Command {
    UPLOAD,
    DOWNLOAD,
    LIST,
    DELETE,
    MKSITE,
    INIT,
  };

  const std::map<std::string, int> command {
    {"upload", Command::UPLOAD},
    {"download", Command::DOWNLOAD},
    {"list", Command::LIST},
    {"delete", Command::DELETE},
    {"mksite", Command::MKSITE},
    {"init", Command::INIT},
  };

  const auto arg1 = parser.next();
  if (command.find(arg1) == command.end()) {
    std::cerr << "Unknown command, see usage" << std::endl;
    return 1;
  }
  // std::cout << command.at(next) << std::endl;
  if (command.at(arg1) != Command::INIT) {
    if (!cl.init()) {
      std::cerr << "Can not initialise 'cloud::Cloud' instance" << std::endl;
      return 1;
    }
  }
  int returnCode = 0;
  switch(command.at(arg1)) {
  case Command::INIT:
    returnCode = init(cl);
    if (returnCode != 0) {
      std::cerr << "Can not init" << std::endl;
    }
    break;
  case Command::UPLOAD:
    returnCode = upload(parser, cl);
    if (returnCode != 0) {
      std::cerr << "Can not upload" << std::endl;
    }
    break;
  case Command::DOWNLOAD:
    returnCode = download(parser, cl);
    if (returnCode != 0) {
      std::cerr << "Can not download" << std::endl;
    }
    break;
  case Command::LIST:
    returnCode = list(parser, cl);
    if (returnCode != 0) {
      std::cerr << "Can not list" << std::endl;
    }
    break;
  case Command::DELETE:
    returnCode = del(parser, cl);
    if (returnCode != 0) {
      std::cerr << "Can not delete" << std::endl;
    }
    break;
  case Command::MKSITE:
    returnCode = mksite(cl);
    if (returnCode != 0) {
      std::cerr << "Can not mksite" << std::endl;
    }
    break;
  default:
    std::cerr << "Unknown command" << std::endl;
    return 1;
  }

  if (!cl.deinit()) {
    std::cerr << "Bad deinitialization" << std::endl;
    return 1;
  }
  return returnCode;
}
