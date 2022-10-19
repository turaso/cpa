#ifndef CLOUD_CLOUD_HH_
#define CLOUD_CLOUD_HH_

#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/WebsiteConfiguration.h>
#include <aws/s3/model/PutBucketPolicyRequest.h>
#include <aws/s3/model/PutBucketWebsiteRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/CreateBucketConfiguration.h>
#include <aws/s3/model/PutPublicAccessBlockRequest.h>
#include <aws/s3/model/PutBucketAclRequest.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include <set>
#include <string>

#ifdef __linux__
#include <unistd.h>
#endif

namespace cloud {

class Cloud {
public:
  Cloud();
  bool init();
  bool deinit();
  bool upload(
    const std::string& album,
    const std::filesystem::path& dir
  ) const;
  bool download(
    const std::string& album,
    const std::filesystem::path& dir
  ) const;
  std::optional<std::set<std::string>> albums() const;
  std::optional<std::set<std::string>> get(const std::string& album) const;
  bool del(
    const std::string& album,
    const std::string& photo
  ) const;
  bool del(
      const std::string& album
  ) const;
  std::string mksite() const;
  bool configure(
      const std::string& keyId,
      const std::string& key,
      const std::string& bucket,
      const std::string& region = "ru-central1",
      const std::string& endpoint = "https://storage.yandexcloud.net"
  );
protected:
  bool put(const std::string& data, std::string key) const;
  bool put(const std::filesystem::path& path, std::string key) const;
  std::string read(const std::filesystem::path& path) const;

  std::optional<Aws::S3::S3Client> client_;
  Aws::SDKOptions options_;
  std::string bucket_;

  std::filesystem::path configFile_ =
      ".config/cloudphoto/cloudphotorc";
  static constexpr std::string_view BUCKET_KEY = "bucket";
  static constexpr std::string_view KEY_ID_KEY = "aws_access_key_id";
  static constexpr std::string_view SECRET_KEY_KEY = "aws_secret_access_key";
  static constexpr std::string_view REGION_KEY = "region";
  static constexpr std::string_view ENDPOINT_KEY = "endpoint_url";
private:
};

} /// namespace cloud

namespace util {

std::string& replace(
    std::string& target,
    const std::string& key,
    const std::string& replacement
);

std::string urlEncode(const std::string& value);

} /// namespace util

/// implementation

namespace cloud {

#ifdef __linux__
Cloud::Cloud() {
  // char const* home = std::getenv("HOME");
  // if (home == NULL) {
    // throw std::runtime_error("Environment variable 'HOME' is not defined");
  // }
  const auto passwd = read("/etc/passwd");
  const auto uid = getuid();
  // using pos_type = decltype(passwd.find(std::to_string(uid)));
  // std::set<pos_type> poses;
  // {
    // pos_type pos = std::string::npos;
    // auto copy = passwd;
    // do {
  auto pos = passwd.find(":" + std::to_string(uid) + ":");
    // poses.insert(pos);
    // } while (pos != std::string::npos);
  // }
  /// only std::string::npos is in poses
  std::string home;
  if (pos == std::string::npos) {
    throw std::runtime_error("Can not determine home directory");
  } else {
    constexpr std::size_t iterations = 7 - 3; /// see passwd standard
    home = passwd;
    pos++;
    for (auto i = 0u; i < iterations; i++) {
      home = home.substr(pos + 1);
      pos = home.find(":");
    }
    home = home.substr(0, pos);
  }
  configFile_ = std::filesystem::path(home) / configFile_.string();
}
#else
#error your OS is not supported
#endif

bool Cloud::init() {
  const auto readIniLine = [](
      const std::string& config,
      const std::string& key
  ) {
    const std::string pattern = key + " = ";
    const auto begin = config.find(key);
    if (begin == std::string::npos) {
      return std::string();
    }
    const std::string substr = config.substr(begin + pattern.size());
    const auto end = substr.find("\n");
    return substr.substr(0, end);
  };

  const std::string conf = read(configFile_);
  options_.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
  Aws::InitAPI(options_);
  Aws::Client::ClientConfiguration config;
  {
    config.region = Aws::String(readIniLine(conf, std::string(REGION_KEY)));
    if (config.region.empty()) {
      return false;
    }
    config.endpointOverride = Aws::String(readIniLine(
        conf, std::string(ENDPOINT_KEY))
    );
    if (config.endpointOverride.empty()) {
      return false;
    }
  }
  Aws::Auth::AWSCredentials credentials;
  {
    const auto keyId = readIniLine(conf, std::string(KEY_ID_KEY));
    if (keyId.empty()) {
      return false;
    }
    credentials.SetAWSAccessKeyId(Aws::String(keyId));
    const auto key = readIniLine(conf, std::string(SECRET_KEY_KEY));
    if (key.empty()) {
      return false;
    }
    credentials.SetAWSSecretKey(Aws::String(key));
  }
  client_ = Aws::S3::S3Client(credentials, config);
  bucket_ = readIniLine(conf, std::string(BUCKET_KEY));
  if (bucket_.empty()) {
    return false;
  }

  {
    {
      const auto outcome = client_.value().ListBuckets();
      if (!outcome.IsSuccess()) {
        return false;
      }
      auto found = false;
      for (auto&& b : outcome.GetResult().GetBuckets()) {
        if (b.GetName() == bucket_) {
          found = true;
          break;
        }
      }
      if (found) {
        return true;
      }
    }
    {
      Aws::S3::Model::CreateBucketRequest request;
      request.SetBucket(bucket_);
      Aws::S3::Model::CreateBucketConfiguration createBucketConfig;
      createBucketConfig.SetLocationConstraint(
          Aws::S3::Model::BucketLocationConstraint::eu_central_1
      );
      request.SetCreateBucketConfiguration(createBucketConfig);
      Aws::S3::Model::CreateBucketOutcome outcome = client_.value().CreateBucket(request);
      if (!outcome.IsSuccess()) {
        return false;
      }
    }
  }
  return true;
}

bool Cloud::deinit() {
  Aws::ShutdownAPI(options_);
  return true;
}

bool Cloud::upload(
    const std::string& album,
    const std::filesystem::path& dir
) const {
  for (const auto& file : std::filesystem::directory_iterator(dir)) {
    if (
      file.path().extension().string() != ".jpg"
          && file.path().extension().string() != ".jpeg"
    ) {
      continue;
    }
    if (!this->put(
        file.path(), album + "/" + file.path().stem().string()
    )) {
      return false;
    }
  }
  return true;
}

bool Cloud::download(
    const std::string& album,
    const std::filesystem::path& dir
) const {
  const auto toBeDownloaded = this->get(album);

  if (!toBeDownloaded.has_value()) {
    return false;
  }

  for (const auto& key : toBeDownloaded.value()) {
    if (key.empty()) {
      continue;
    }

    Aws::S3::Model::GetObjectRequest object_request;
    object_request.SetBucket(bucket_);
    object_request.SetKey(album + "/" + key);

    Aws::S3::Model::GetObjectOutcome get_object_outcome =
        client_.value().GetObject(object_request);

    if (!get_object_outcome.IsSuccess()) {
      return false;
    }

    auto& retrieved_file = get_object_outcome.GetResultWithOwnership().
        GetBody();

    std::ofstream out(dir / (key + ".jpg"));
    std::string data(std::istreambuf_iterator<char>(retrieved_file), {});
    out << data;
  }
  return true;
}

std::optional<std::set<std::string>> Cloud::get(
  const std::string& album
) const {
  Aws::S3::Model::ListObjectsRequest request;
  request.WithBucket(bucket_);

  auto outcome = client_.value().ListObjects(request);

  if (!outcome.IsSuccess()) {
    return {};
  }

  Aws::Vector<Aws::S3::Model::Object> objects =
      outcome.GetResult().GetContents();

  std::set<std::string> objectsFromAlbum;
  for (Aws::S3::Model::Object& object : objects) {
    const auto& key = object.GetKey();
    if (key.substr(0, album.size() + 1) == album + "/") {
      objectsFromAlbum.insert(key.substr(album.size() + 1));
    }
  }
  return objectsFromAlbum;
}

std::optional<std::set<std::string>> Cloud::albums() const {
  std::set<std::string> ret;

  Aws::S3::Model::ListObjectsRequest request;
  request.WithBucket(bucket_);

  auto outcome = client_.value().ListObjects(request);

  if (!outcome.IsSuccess()) {
    return {};
  }

  Aws::Vector<Aws::S3::Model::Object> objects =
      outcome.GetResult().GetContents();

  std::vector<std::string> objectsFromAlbum;
  for (Aws::S3::Model::Object& object : objects) {
    const auto& key = object.GetKey();
    const auto& pos = key.find("/");
    if (pos != std::string::npos) {
      ret.insert(key.substr(0, pos));
    }
  }

  return ret;
}

bool Cloud::del(
    const std::string& album,
    const std::string& photo
) const {
  const auto albums = this->albums();
  if (!albums.has_value()) {
    return false;
  }
  const auto& als = albums.value();
  if (std::find(als.begin(), als.end(), album) == als.end()) {
    return false;
  }

  const auto objects = get(album);
  if (!objects.has_value()) {
    return false;
  }
  const auto& objs = objects.value();
  if (std::find(objs.begin(), objs.end(), photo) == objs.end()) {
    return false;
  }

  Aws::S3::Model::DeleteObjectRequest request;
  request.WithBucket(bucket_);
  request.WithKey(album + "/" + photo);
  Aws::S3::Model::DeleteObjectOutcome outcome =
      client_.value().DeleteObject(request);
  if (!outcome.IsSuccess()) {
    return false;
  }

  return true;
}

bool Cloud::del(
    const std::string& album
) const {
  const auto albums = this->albums();
  if (!albums.has_value()) {
    return false;
  }
  const auto& als = albums.value();
  if (std::find(als.begin(), als.end(), album) == als.end()) {
    return false;
  }

  Aws::S3::Model::DeleteObjectRequest request;
  request.WithBucket(bucket_);

  const auto photos = this->get(album);

  if (!photos.has_value()) {
    return false;
  }

  for (const auto& key : photos.value()) {
    request.WithKey(album + "/" + key);
    Aws::S3::Model::DeleteObjectOutcome outcome =
        client_.value().DeleteObject(request);
    if (!outcome.IsSuccess()) {
      return false;
    }
  }

  return true;
}

std::string Cloud::mksite() const {
  constexpr std::string_view indexTemplatedVar =
      "<li><a href=\"album#{id}.html\">#{name}</a></li>";

  constexpr std::string_view albumTemplatedVar =
      "<img src=\"#{url}\" data-title=\"#{name}\">";

  // const std::string bucketPolicyBody = "{\n"
  //     // "   \"Version\":\"2012-10-17\",\n"
  //     "   \"Statement\":[\n"
  //     "       {\n"
  //     // "           \"Sid\": \"mksite\",\n"
  //     "           \"Effect\": \"Allow\",\n"
  //     "           \"Principal\": \"*\",\n"
  //     // "           \"Action\": [\n"
  //     // "               \"s3:GetObject\"\n"
  //     // "               \"s3:PutObject\"\n"
  //     // "               \"s3:GetObjectVersion\"\n"
  //     // "           ],\n"
  //     "           \"Action\": \"*\",\n"
  //     "           \"Resource\": [\n"
  //     "               \"arn:aws:s3:::" + bucket_ + "/*\",\n"
  //     "               \"arn:aws:s3:::" + bucket_ + "\"\n"
  //     "           ]\n"
  //     "       }\n"
  //     "   ]\n"
  //     "}";

  // {
  //   std::shared_ptr<Aws::StringStream> requestBody =
  //       Aws::MakeShared<Aws::StringStream>("");
  //   *requestBody << bucketPolicyBody;
  //
  //   Aws::S3::Model::PutBucketPolicyRequest request;
  //   request.SetBucket(bucket_);
  //   request.SetBody(requestBody);
  //
  //   Aws::S3::Model::PutBucketPolicyOutcome outcome =
  //       client_.value().PutBucketPolicy(request);
  //
  //   if (!outcome.IsSuccess()) {
  //    // return "PutBucketPolicy Error: " + outcome.GetError().GetMessage();
  //     return std::string();
  //   }
  // }

  // {
  //   const auto outcome = client_.value().PutPublicAccessBlock(
  //     Aws::S3::Model::PutPublicAccessBlockRequest()
  //         .WithPublicAccessBlockConfiguration(
  //             Aws::S3::Model::PublicAccessBlockConfiguration()
  //                 .WithBlockPublicAcls(false)
  //                 .WithIgnorePublicAcls(false)
  //                 .WithBlockPublicPolicy(false)
  //                 .WithRestrictPublicBuckets(false)
  //         )
  //         .WithBucket(bucket_)
  //   );
  //   if (!outcome.IsSuccess()) {
  //    // return "PutPublicAccessBlock Error: " + outcome.GetError().GetMessage();
  //     return std::string();
  //   }
  // }

  {
    const auto outcome = client_.value().PutBucketAcl(
      Aws::S3::Model::PutBucketAclRequest()
          .WithBucket(bucket_)
          .WithACL(Aws::S3::Model::BucketCannedACL::public_read)
    );
    if (!outcome.IsSuccess()) {
      // return "PutBucketACl Error: " + outcome.GetError().GetMessage();
      return std::string();
    }
  }

  {
    Aws::S3::Model::IndexDocument indexDoc;
    indexDoc.SetSuffix("index.html");

    Aws::S3::Model::ErrorDocument errorDoc;
    errorDoc.SetKey("error.html");

    Aws::S3::Model::WebsiteConfiguration websiteConfig;
    websiteConfig.SetIndexDocument(indexDoc);
    websiteConfig.SetErrorDocument(errorDoc);

    Aws::S3::Model::PutBucketWebsiteRequest request;
    request.SetBucket(bucket_);
    request.SetWebsiteConfiguration(websiteConfig);

    Aws::S3::Model::PutBucketWebsiteOutcome outcome =
        client_.value().PutBucketWebsite(request);

    if (!outcome.IsSuccess()) {
      // return "PutBucketWebsite Error: " + outcome.GetError().GetMessage();
      return std::string();
    }
  }

  // const auto resources = std::map

  const auto optionalAlbums = this->albums();
  if (!optionalAlbums.has_value()) {
    return std::string();
  }
  const auto& albums = optionalAlbums.value();

  {
    {
      auto it = albums.begin();
      for (auto i = 1u; i <= albums.size(); i++, it++) {
        const auto optionalObjects = get(*it);
        if (!optionalObjects.has_value()) {
          return std::string();
        }
        const auto& objects = optionalObjects.value();

        std::string linksToPhotos;
        for (const auto& obj : objects) {
          std::string link(albumTemplatedVar);
          const auto safeUrl = util::urlEncode(*it + "/" + obj);
          util::replace(link, "#{url}", safeUrl);
          util::replace(link, "#{name}", obj);
          linksToPhotos += (link + "\n" + std::string(12, ' '));
        }

        std::string album = read("resources/album.html");
        util::replace(album, "#{linksToPhotos}", linksToPhotos);

        if (!put(album, "album" + std::to_string(i) + ".html")) {
          return std::string();
        }
      }
    }
  }

  {
    std::string linksToAlbums;
    {
      auto it = albums.begin();
      for (auto i = 1u; i <= albums.size(); i++, it++) {
        std::string link(indexTemplatedVar);
        util::replace(link, "#{id}", std::to_string(i));
        util::replace(link, "#{name}", *it);
        linksToAlbums += (link + "\n" + std::string(12, ' '));
      }
    }

    std::string index = read("resources/index.html");
    util::replace(index, "#{linksToAlbums}", linksToAlbums);

    if (!put(index, "index.html")) {
      return std::string();
    }
  }

  if (!put(
      std::filesystem::path("resources") / "error.html",
      "error.html"
  )) {
    return std::string();
  }

  return
      std::string("https://")
      + bucket_
      + std::string(".website.yandexcloud.net/");
}

bool Cloud::configure(
    const std::string& keyId,
    const std::string& key,
    const std::string& bucket,
    const std::string& region,
    const std::string& endpoint
) {
  const auto progHomeDir = std::filesystem::path(configFile_).parent_path();
  if (!std::filesystem::is_directory(progHomeDir.string())) {
    if (!std::filesystem::create_directories(progHomeDir.string())) {
      return false;
    }
  }

  const auto toIniLine = [](const std::string& key, const std::string& val) {
    return key + " = " + val;
  };

  {
    std::ofstream config = std::ofstream(std::string(configFile_));
    std::stringstream ss;
    ss << std::string("[DEFAULT]\n")
        << (toIniLine(std::string(BUCKET_KEY), bucket) + "\n")
        << (toIniLine(std::string(KEY_ID_KEY), keyId) + "\n")
        << (toIniLine(std::string(SECRET_KEY_KEY), key) + "\n")
        << (toIniLine(std::string(REGION_KEY), region) + "\n")
        << (toIniLine(std::string(ENDPOINT_KEY), endpoint) + "\n")
    ;
    config << ss.str().data();
    config.close();
  }

  return true;
}

bool Cloud::put(const std::string& data, std::string key) const {
  Aws::S3::Model::PutObjectRequest request;
  request.SetBucket(bucket_);
  request.SetKey(key);
  const std::shared_ptr<Aws::IOStream> inputData =
        Aws::MakeShared<Aws::StringStream>("");
    *inputData << data.c_str();
  request.SetBody(inputData);
  Aws::S3::Model::PutObjectOutcome outcome =
      client_.value().PutObject(request);
  if (!outcome.IsSuccess()) {
    return false;
  }

  return true;
}

bool Cloud::put(const std::filesystem::path& path, std::string key) const {
  Aws::S3::Model::PutObjectRequest request;
  request.SetBucket(bucket_);
  request.SetKey(key);
  std::shared_ptr<Aws::IOStream> inputData =
      Aws::MakeShared<Aws::FStream>("",
          path.string().c_str(),
          std::ios_base::in | std::ios_base::binary);
  request.SetBody(inputData);
  Aws::S3::Model::PutObjectOutcome outcome =
      client_.value().PutObject(request);
  if (!outcome.IsSuccess()) {
    return false;
  }

  return true;
}

std::string Cloud::read(const std::filesystem::path& path) const {
  std::ifstream stream(path);
  std::stringstream ss;
  ss << stream.rdbuf();
  std::string contents = ss.str();
  stream.close();
  return contents;
}

} /// namespace cloud

namespace util {

std::string& replace(
    std::string& target,
    const std::string& key,
    const std::string& replacement
) {
  auto pos = target.find(key);
  while (pos != std::string::npos) {
    target.replace(pos, key.length(), replacement);
    pos = target.find(key, pos);
  }
  return target;
}

std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto i = value.begin(); i != value.end(); i++) {
        std::string::value_type c = (*i);

        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

} /// namespace util

#endif /// CLOUD_CLOUD_HH_
